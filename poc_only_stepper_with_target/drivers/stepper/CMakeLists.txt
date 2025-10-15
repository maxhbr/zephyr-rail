# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SIMPLE_STEPPER)
	target_sources(app PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/simple_stepper.c
		${ZEPHYR_BASE}/drivers/stepper/step_dir/step_dir_stepper_common.c
		${ZEPHYR_BASE}/drivers/stepper/step_dir/step_dir_stepper_work_timing.c
	)
endif()
