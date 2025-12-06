#include "shell.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(shell, LOG_LEVEL_INF);

static int cmd_rail_go(const struct shell *sh, size_t argc, char **argv) {

  if (argc != 2) {
    shell_print(sh, "Usage: rail %s <distance>", argv[0]);
    return -EINVAL;
  }

  // Check if argv[0] is `go` or `go_nm`:
  bool is_nm_command = (strcmp(argv[0], "go_nm") == 0);
  int multiplier =
      is_nm_command ? 1 : 1000; // go_nm uses nm directly, go uses um

  int distance_nm = atof(argv[1]) * multiplier;
  event_pub(EVENT_GO, distance_nm);

  return 0;
}

static int cmd_rail_go_to(const struct shell *sh, size_t argc, char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail go_to <absolute_position>");
    return -EINVAL;
  }

  int position_um = atof(argv[1]);
  int position_nm = position_um * 1000;
  event_pub(EVENT_GO_TO, position_nm);

  return 0;
}

static int cmd_rail_go_pct(const struct shell *sh, size_t argc, char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail go_pct <percentage>");
    return -EINVAL;
  }

  int percentage = atoi(argv[1]);
  if (percentage < 0 || percentage > 100) {
    shell_print(sh, "Percentage must be between 0 and 100");
    return -EINVAL;
  }

  event_pub(EVENT_GO_PCT, percentage);

  return 0;
}

static int cmd_rail_setLowerBound(const struct shell *sh, size_t argc,
                                  char **argv) {
  if (argc == 2) {
    int position_um = atof(argv[1]);
    int position_nm = position_um * 1000;
    event_pub(EVENT_SET_LOWER_BOUND_TO, position_nm);
  } else {
    event_pub(EVENT_SET_LOWER_BOUND);
  }
  return 0;
}

static int cmd_rail_setUpperBound(const struct shell *sh, size_t argc,
                                  char **argv) {
  if (argc == 2) {
    int position_um = atof(argv[1]);

    int position_nm = position_um * 1000;
    event_pub(EVENT_SET_UPPER_BOUND_TO, position_nm);
  } else {
    event_pub(EVENT_SET_UPPER_BOUND);
  }
  return 0;
}

static int cmd_rail_setWaitBefore(const struct shell *sh, size_t argc,
                                  char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail wait_before <milliseconds>");
    return -EINVAL;
  }

  int wait_ms = atoi(argv[1]);
  event_pub(EVENT_SET_WAIT_BEFORE_MS, wait_ms);
  return 0;
}

static int cmd_rail_setWaitAfter(const struct shell *sh, size_t argc,
                                 char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail wait_after <milliseconds>");
    return -EINVAL;
  }

  int wait_ms = atoi(argv[1]);
  event_pub(EVENT_SET_WAIT_AFTER_MS, wait_ms);
  return 0;
}

static int cmd_rail_setSpeedRpm(const struct shell *sh, size_t argc,
                                char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail set_rpm <rpm>");
    return -EINVAL;
  }

  int rpm = atoi(argv[1]);
  if (rpm < 1) {
    shell_print(sh, "RPM must be >= 1");
    return -EINVAL;
  }

  event_pub(EVENT_SET_SPEED_RPM, rpm);
  return 0;
}

static int cmd_rail_setSpeed(const struct shell *sh, size_t argc, char **argv) {
  if (argc != 2) {
    shell_print(sh, "Usage: rail set_speed <slow|medium|fast>");
    return -EINVAL;
  }

  int speed_value = 0;
  if (strcmp(argv[1], "slow") == 0) {
    speed_value = 1;
  } else if (strcmp(argv[1], "medium") == 0) {
    speed_value = 2;
  } else if (strcmp(argv[1], "fast") == 0) {
    speed_value = 3;
  } else {
    shell_print(sh, "Unknown speed '%s'. Use slow, medium, or fast.", argv[1]);
    return -EINVAL;
  }

  event_pub(EVENT_SET_SPEED, speed_value);
  return 0;
}

static int cmd_rail_startStackWithStepSize(const struct shell *sh, size_t argc,
                                           char **argv) {
  bool is_nm_command = (strcmp(argv[0], "stack_nm") == 0);
  int multiplier =
      is_nm_command ? 1 : 1000; // stack_nm uses nm directly, stack uses um

  if (argc == 2) {
    int expected_step_size_nm = atof(argv[1]) * multiplier;
    event_pub(EVENT_START_STACK_WITH_STEP_SIZE, expected_step_size_nm);
  } else if (argc > 2) {
    shell_print(sh, "Usage: rail %s <expected_step_size>", argv[0]);
    return -EINVAL;
  } else {
    event_pub(EVENT_START_STACK_WITH_STEP_SIZE, 1000);
  }
  return 0;
}
static int cmd_rail_startStackWithLength(const struct shell *sh, size_t argc,
                                         char **argv) {
  if (argc == 2) {
    int expected_length_of_stack = atoi(argv[1]);
    event_pub(EVENT_START_STACK_WITH_LENGTH, expected_length_of_stack);
  } else if (argc > 2) {
    shell_print(sh, "Usage: rail startStack <expected_length_of_stack>");
    return -EINVAL;
  } else {
    event_pub(EVENT_START_STACK_WITH_LENGTH, 100);
  }
  return 0;
}

static int cmd_rail_status(const struct shell *sh, size_t argc, char **argv) {
  event_pub(EVENT_STATUS);
  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_rail, SHELL_CMD(go, NULL, "Go relative.", cmd_rail_go),
    SHELL_CMD(g, NULL, "Go relative.", cmd_rail_go),
    SHELL_CMD(go_nm, NULL, "Go relative (nm).", cmd_rail_go),
    SHELL_CMD(go_to, NULL, "Go to absolute position.", cmd_rail_go_to),
    SHELL_CMD(go_pct, NULL, "Go to percentage between upper and lower bound.",
              cmd_rail_go_pct),
    SHELL_CMD(p, NULL, "Go to percentage between upper and lower bound.",
              cmd_rail_go_pct),
    SHELL_CMD(lower, NULL, "Set lower bound.", cmd_rail_setLowerBound),
    SHELL_CMD(upper, NULL, "Set upper bound.", cmd_rail_setUpperBound),
    SHELL_CMD(wait_before, NULL, "Set wait before ms.", cmd_rail_setWaitBefore),
    SHELL_CMD(wait_after, NULL, "Set wait after ms.", cmd_rail_setWaitAfter),
    SHELL_CMD(set_speed, NULL, "Set movement speed (slow|medium|fast).",
              cmd_rail_setSpeed),
    SHELL_CMD(set_rpm, NULL, "Set movement speed using raw RPM.",
              cmd_rail_setSpeedRpm),
    SHELL_CMD(s, NULL, "Start stacking with step size.",
              cmd_rail_startStackWithStepSize),
    SHELL_CMD(stack, NULL, "Start stacking with step size.",
              cmd_rail_startStackWithStepSize),
    SHELL_CMD(stack_nm, NULL, "Start stacking with step size (nm).",
              cmd_rail_startStackWithStepSize),
    SHELL_CMD(stack_count, NULL, "Start stacking with length.",
              cmd_rail_startStackWithLength),
    SHELL_CMD(status, NULL, "Get current status.", cmd_rail_status),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(rail, &sub_rail, "rail commands", NULL);
SHELL_CMD_REGISTER(r, &sub_rail, "rail commands", NULL);

static int cmd_cam_shoot(const struct shell *sh, size_t argc, char **argv) {
  event_pub(EVENT_SHOOT);
  return 0;
}

static int cmd_cam_record(const struct shell *sh, size_t argc, char **argv) {
  event_pub(EVENT_RECORD);
  return 0;
}

static int cmd_cam_startScan(const struct shell *sh, size_t argc, char **argv) {
  event_pub(EVENT_CAMERA_START_SCAN);
  return 0;
}

static int cmd_cam_stopScan(const struct shell *sh, size_t argc, char **argv) {
  event_pub(EVENT_CAMERA_STOP_SCAN);
  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_cam, SHELL_CMD(shoot, NULL, "Trigger camera shoot.", cmd_cam_shoot),
    SHELL_CMD(record, NULL, "Toggle camera recording.", cmd_cam_record),
    SHELL_CMD(scan, NULL, "Scan for camera", cmd_cam_startScan),
    SHELL_CMD(stopScan, NULL, "Stop scan for camera", cmd_cam_stopScan),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(cam, &sub_cam, "cam commands", NULL);
SHELL_CMD_REGISTER(c, &sub_cam, "cam commands", NULL);
