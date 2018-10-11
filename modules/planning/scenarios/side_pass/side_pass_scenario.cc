/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/**
 * @file
 **/

#include "modules/planning/scenarios/side_pass/side_pass_scenario.h"

#include <fstream>
#include <limits>
#include <utility>

#include "cybertron/common/log.h"
#include "modules/common/math/math_utils.h"
#include "modules/common/time/time.h"
#include "modules/common/util/file.h"
#include "modules/common/util/string_tokenizer.h"
#include "modules/common/util/string_util.h"
#include "modules/common/vehicle_state/vehicle_state_provider.h"
#include "modules/map/hdmap/hdmap.h"
#include "modules/map/hdmap/hdmap_common.h"
#include "modules/planning/common/ego_info.h"
#include "modules/planning/common/frame.h"
#include "modules/planning/common/planning_gflags.h"
#include "modules/planning/constraint_checker/constraint_checker.h"
#include "modules/planning/toolkits/optimizers/dp_poly_path/dp_poly_path_optimizer.h"
#include "modules/planning/toolkits/optimizers/dp_st_speed/dp_st_speed_optimizer.h"
#include "modules/planning/toolkits/optimizers/path_decider/path_decider.h"
#include "modules/planning/toolkits/optimizers/poly_st_speed/poly_st_speed_optimizer.h"
#include "modules/planning/toolkits/optimizers/qp_spline_path/qp_spline_path_optimizer.h"
#include "modules/planning/toolkits/optimizers/qp_spline_st_speed/qp_spline_st_speed_optimizer.h"
#include "modules/planning/toolkits/optimizers/speed_decider/speed_decider.h"

namespace apollo {
namespace planning {

using common::ErrorCode;
using common::SLPoint;
using common::SpeedPoint;
using common::Status;
using common::TrajectoryPoint;
using common::math::Vec2d;
using common::time::Clock;

namespace {
constexpr double kPathOptimizationFallbackCost = 2e4;
constexpr double kSpeedOptimizationFallbackCost = 2e4;
constexpr double kStraightForwardLineCost = 10.0;
}  // namespace

void SidePassScenario::RegisterTasks() {
  task_factory_.Register(DP_POLY_PATH_OPTIMIZER,
                         []() -> Task* { return new DpPolyPathOptimizer(); });
  task_factory_.Register(PATH_DECIDER,
                         []() -> Task* { return new PathDecider(); });
  task_factory_.Register(DP_ST_SPEED_OPTIMIZER,
                         []() -> Task* { return new DpStSpeedOptimizer(); });
  task_factory_.Register(SPEED_DECIDER,
                         []() -> Task* { return new SpeedDecider(); });
  task_factory_.Register(QP_SPLINE_ST_SPEED_OPTIMIZER, []() -> Task* {
    return new QpSplineStSpeedOptimizer();
  });
}

bool SidePassScenario::Init() {
  if (is_init_) {
    return true;
  }

  CHECK(apollo::common::util::GetProtoFromFile(
      FLAGS_scenario_side_pass_config_file, &config_));

  RegisterTasks();

  is_init_ = true;
  status_ = STATUS_INITED;

  return true;
}

Status SidePassScenario::Process(const TrajectoryPoint& planning_start_point,
                                 Frame* frame) {
  status_ = STATUS_PROCESSING;

  const int current_stage_index = StageIndexInConf(stage_);
  if (!InitTasks(config_, current_stage_index, &tasks_)) {
    return Status(ErrorCode::PLANNING_ERROR, "failed to init tasks");
  }

  // TODO(all)

  Status status = Status(ErrorCode::PLANNING_ERROR,
                         "Failed to process stage in side pass.");
  switch (stage_) {
    case SidePassStage::OBSTACLE_APPROACH: {
      status = ApproachObstacle(planning_start_point, frame);
      break;
    }
    case SidePassStage::PATH_GENERATION: {
      status = GeneratePath(planning_start_point, frame);
      break;
    }
    case SidePassStage::WAITPOINT_STOP: {
      status = StopOnWaitPoint(planning_start_point, frame);
      break;
    }
    case SidePassStage::SAFETY_DETECTION: {
      status = DetectSafety(planning_start_point, frame);
      break;
    }
    case SidePassStage::OBSTACLE_PASS: {
      status = PassObstacle(planning_start_point, frame);
      break;
    }
    default:
      break;
  }

  return status;
}

bool SidePassScenario::IsTransferable(const Scenario& current_scenario,
                                      const common::TrajectoryPoint& ego_point,
                                      const Frame& frame) const {
  if (frame.reference_line_info().size() > 1) {
    return false;
  }
  if (current_scenario.scenario_type() == ScenarioConfig::SIDE_PASS) {
    return (current_scenario.GetStatus() !=
            Scenario::ScenarioStatus::STATUS_DONE);
  } else if (current_scenario.scenario_type() != ScenarioConfig::LANE_FOLLOW) {
    return false;
  } else {
    return IsSidePassScenario(ego_point, frame);
  }
}

int SidePassScenario::StageIndexInConf(const SidePassStage& stage) {
  // note: this is the index in scenario conf file.  must be consistent
  if (stage == SidePassStage::OBSTACLE_APPROACH) {
    return 0;
  } else if (stage == SidePassStage::PATH_GENERATION) {
    return 1;
  } else if (stage == SidePassStage::WAITPOINT_STOP) {
    return 2;
  } else if (stage == SidePassStage::SAFETY_DETECTION) {
    return 3;
  } else if (stage == SidePassStage::OBSTACLE_PASS) {
    return 4;
  }
  return -1;
}

Status SidePassScenario::ApproachObstacle(
    const TrajectoryPoint& planning_start_point, Frame* frame) {
  if (!stage_init_) {
    // TODO(yifei) init stage
    stage_init_ = true;
  }
  Status status = RunPlanOnReferenceLine(planning_start_point, frame);
  if (status.ok()) {
    if (!IsSidePassScenario(planning_start_point, *frame)) {
      // TODO(yifei) scenario is done
    } else if (frame->vehicle_state().linear_velocity() < 1.0e-5) {
      stage_ = SidePassStage::PATH_GENERATION;
      stage_init_ = false;
    }
  }
  return status;
}

Status SidePassScenario::GeneratePath(
    const TrajectoryPoint& planning_start_point, Frame* frame) {
  return Status::OK();
}

Status SidePassScenario::StopOnWaitPoint(
    const TrajectoryPoint& planning_start_point, Frame* frame) {
  return Status::OK();
}

Status SidePassScenario::DetectSafety(
    const TrajectoryPoint& planning_start_point, Frame* frame) {
  return Status::OK();
}

Status SidePassScenario::PassObstacle(
    const TrajectoryPoint& planning_start_point, Frame* frame) {
  return Status::OK();
}

bool SidePassScenario::IsSidePassScenario(
    const common::TrajectoryPoint& planning_start_point,
    const Frame& frame) const {
  // TODO(lianglia-apollo)
  const SLBoundary& adc_sl_boundary =
      frame.reference_line_info().front().AdcSlBoundary();
  const PathDecision& path_decision =
      frame.reference_line_info().front().path_decision();
  return HasBlockingObstacle(adc_sl_boundary, path_decision);
}

bool SidePassScenario::HasBlockingObstacle(
    const SLBoundary& adc_sl_boundary,
    const PathDecision& path_decision) const {
  // a blocking obstacle is an obstacle blocks the road when it is not blocked
  // (by other obstacles or traffic rules)
  for (const auto* path_obstacle : path_decision.path_obstacles().Items()) {
    if (path_obstacle->obstacle()->IsVirtual() ||
        !path_obstacle->obstacle()->IsStatic()) {
      continue;
    }
    CHECK(path_obstacle->obstacle()->IsStatic());

    if (path_obstacle->PerceptionSLBoundary().start_s() <=
        adc_sl_boundary.end_s()) {  // such vehicles are behind the adc.
      continue;
    }
    constexpr double kAdcDistanceThreshold = 15.0;  // unit: m
    if (path_obstacle->PerceptionSLBoundary().start_s() >
        adc_sl_boundary.end_s() +
            kAdcDistanceThreshold) {  // vehicles are far away
      continue;
    }
    if (path_obstacle->PerceptionSLBoundary().start_l() > 1.0 ||
        path_obstacle->PerceptionSLBoundary().end_l() < -1.0) {
      continue;
    }

    bool is_blocked_by_others = false;
    for (const auto* other_obstacle : path_decision.path_obstacles().Items()) {
      if (other_obstacle->Id() == path_obstacle->Id()) {
        continue;
      }
      if (other_obstacle->PerceptionSLBoundary().start_l() >
              path_obstacle->PerceptionSLBoundary().end_l() ||
          other_obstacle->PerceptionSLBoundary().end_l() <
              path_obstacle->PerceptionSLBoundary().start_l()) {
        // not blocking the backside vehicle
        continue;
      }

      double delta_s = other_obstacle->PerceptionSLBoundary().start_s() -
                       path_obstacle->PerceptionSLBoundary().end_s();
      if (delta_s < 0.0 || delta_s > kAdcDistanceThreshold) {
        continue;
      } else {
        // TODO(All): fixed the segmentation bug for large vehicles, otherwise
        // the follow line will be problematic.
        // is_blocked_by_others = true; break;
      }
    }
    if (!is_blocked_by_others) {
      return true;
    }
  }
  return false;
}

Status SidePassScenario::RunPlanOnReferenceLine(
    const TrajectoryPoint& planning_start_point, Frame* frame) {
  auto status =
      Status(ErrorCode::PLANNING_ERROR, "reference line not drivable");

  for (auto& reference_line_info : *frame->mutable_reference_line_info()) {
    if (!reference_line_info.IsDrivable()) {
      return status;
    }

    auto ret = Status::OK();
    for (auto& optimizer : tasks_) {
      ret = optimizer->Execute(frame, &reference_line_info);
      if (!ret.ok()) {
        AERROR << "Failed to run tasks[" << optimizer->Name()
               << "], Error message: " << ret.error_message();
        break;
      }
    }

    reference_line_info.set_trajectory_type(ADCTrajectory::NORMAL);
    DiscretizedTrajectory trajectory;
    if (!reference_line_info.CombinePathAndSpeedProfile(
            planning_start_point.relative_time(),
            planning_start_point.path_point().s(), &trajectory)) {
      std::string msg("Fail to aggregate planning trajectory.");
      AERROR << msg;
      return Status(ErrorCode::PLANNING_ERROR, msg);
    }
    reference_line_info.SetTrajectory(trajectory);
    reference_line_info.SetDrivable(true);
    return Status::OK();
  }
  return status;
}

}  // namespace planning
}  // namespace apollo