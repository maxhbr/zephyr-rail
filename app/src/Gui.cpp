#include "Gui.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gui);


int Gui::position_as_nm(const int pitch_per_rev, const int pulses_per_rev, const int position) {
  return (position * pitch_per_rev * 1000) / pulses_per_rev;
}

Gui::Gui() : Display()
{
    move_tab = make_tab("move");
    stack_tab = make_tab("stack");
    config_tab = make_tab("cfg");
    status_tab = make_tab("status");
}

void Gui::update(const struct stepper_with_target_status *stepper_with_target_status,
      const struct stack_status *stack_status)
{
    char buf[1000];
    const struct stepper_status *stepper_status = &stepper_with_target_status->stepper_status;
    const int pitch_per_rev = stepper_status->pitch_per_rev;
    const int pulses_per_rev = stepper_status->pulses_per_rev;

    const int position_nm = position_as_nm(pitch_per_rev, pulses_per_rev, stepper_status->position);

    if (stepper_with_target_status->is_moving)
    {
      const int target_position_nm = position_as_nm(pitch_per_rev, pulses_per_rev, stepper_with_target_status->target_position);
        if (stepper_with_target_status->target_position > stepper_status->position)
        {
            snprintf(buf, sizeof(buf), "%d (-> %d)", position_nm, target_position_nm);
        }
        else
        {
            snprintf(buf, sizeof(buf), "(%d <-) %d", target_position_nm, position_nm);
        }
    }
    else
    {
        snprintf(buf, sizeof(buf), "@%dnm", position_nm);
    }
    Display::set_status(buf);

    const int lower_bound_nm = position_as_nm(pitch_per_rev, pulses_per_rev, stack_status->lower_bound);
    if (stepper_status->position > stack_status->lower_bound) {
      snprintf(buf, sizeof(buf), "%d <<", lower_bound_nm);
    } else if (stepper_status->position == stack_status->lower_bound) {
      snprintf(buf, sizeof(buf), "%d ==", lower_bound_nm);
    } else {
      snprintf(buf, sizeof(buf), "%d >>", lower_bound_nm);
    }
    Display::set_status_left(buf);

    const int upper_bound_nm = position_as_nm(pitch_per_rev, pulses_per_rev, stack_status->upper_bound);
    if (stepper_status->position < stack_status->upper_bound) {
      snprintf(buf, sizeof(buf), "<< %d", upper_bound_nm);
    } else if (stepper_status->position == stack_status->upper_bound) {
      snprintf(buf, sizeof(buf), "== %d", upper_bound_nm);
    } else {
      snprintf(buf, sizeof(buf), ">> %d", upper_bound_nm);
    }
    Display::set_status_right(buf);
}






/* void View::fill_move_panel(lv_obj_t *parent) */
/* { */



/*     /1* step_size_roller = Display::add_roller(parent, "12800\n" *1/ */
/*     /1*                                                "6400\n" *1/ */
/*     /1*                                                "1280\n" *1/ */
/*     /1*                                                "640\n" *1/ */
/*     /1*                                                "128\n" *1/ */
/*     /1*                                                "64\n" *1/ */
/*     /1*                                                "12\n" *1/ */
/*     /1*                                                "6\n" *1/ */
/*     /1*                                                "1"); *1/ */
/*     /1* lv_obj_align(step_size_roller, LV_ALIGN_TOP_MID, 0, 0); *1/ */

/*     lv_obj_t *left_btn = Display::add_button(parent, "<<", 100, 60); */
/*     lv_obj_align(left_btn, LV_ALIGN_TOP_LEFT, 10, 10); */
/*     lv_obj_add_event_cb( */
/*         left_btn, [](lv_event_t *event) */
/*         { */ 
/*             const struct controller_msg msg = {GO_CONTROLLER_ACTION, -12800}; */
/*             static_view_pointer->controller->handle_controller_msg(&msg); }, */
/*         LV_EVENT_PRESSED, NULL); */
/*     lv_obj_t *right_btn = Display::add_button(parent, ">>", 100, 60); */
/*     lv_obj_align(right_btn, LV_ALIGN_TOP_RIGHT, -10, 10); */
/*     lv_obj_add_event_cb( */
/*         right_btn, [](lv_event_t *event) */
/*         { */ 
/*             const struct controller_msg msg = {GO_CONTROLLER_ACTION, +12800}; */
/*             static_view_pointer->controller->handle_controller_msg(&msg); }, */
/*         LV_EVENT_PRESSED, NULL); */

/*     lv_obj_t *set_lower = Display::add_button(parent, "Set lower", 100, 30); */
/*     lv_obj_align(set_lower, LV_ALIGN_TOP_LEFT, 10, 75); */
/*     //   lv_obj_add_event_cb(set_lower, [](lv_obj_t *btn, lv_event_t event) { */
/*     //     //static_view_pointer->event_cb(ACTION_SET_LOWER, btn, event); */
/*     //   }); */
/*     lv_obj_t *set_upper = Display::add_button(parent, "Set upper", 100, 30); */
/*     lv_obj_align(set_upper, LV_ALIGN_TOP_RIGHT, -10, 75); */
/*     //   lv_obj_add_event_cb(set_upper, [](lv_obj_t *btn, lv_event_t event) { */
/*     //     //static_view_pointer->event_cb(ACTION_SET_UPPER, btn, event); */
/*     //   }); */

/*     lv_obj_t *go_to_lower = Display::add_button(parent, NULL, 100, 30); */
/*     lower_label = Display::add_label(go_to_lower); */
/*     lv_label_set_text(lower_label, "Go to lower"); */
/*     lv_obj_align(go_to_lower, LV_ALIGN_TOP_LEFT, 10, 110); */
/*     //   lv_obj_add_event_cb(go_to_lower, [](lv_obj_t *btn, lv_event_t event) { */
/*     //     //static_view_pointer->event_cb(ACTION_GO_TO_LOWER, btn, event); */
/*     //   }); */

/*     lv_obj_t *go_to_upper = Display::add_button(parent, NULL, 100, 30); */
/*     upper_label = Display::add_label(go_to_upper); */
/*     lv_label_set_text(upper_label, "Go to upper"); */
/*     lv_obj_align(go_to_upper, LV_ALIGN_TOP_RIGHT, -10, 110); */
/*     //   lv_obj_add_event_cb(go_to_upper, [](lv_obj_t *btn, lv_event_t event) { */
/*     //     //static_view_pointer->event_cb(ACTION_GO_TO_UPPER, btn, event); */
/*     //   }); */

/*     // lv_obj_t *slider = lv_slider_create(parent, NULL); */
/*     // lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -10); */
/*     // lv_slider_set_range(slider, 0, 100); */
/*     // lv_slider_set_value(slider, 50, 0); */
/*     // // lv_obj_set_width(slider, LV_PCT(95)) */
/* } */

/* void View::fill_stack_panel(lv_obj_t *parent) */
/* { */
/* #if 0 */
/*   step_number_roller = Display::add_roller(parent, "10\n" */
/*                                                    "20\n" */
/*                                                    "30\n" */
/*                                                    "50\n" */
/*                                                    "100\n" */
/*                                                    "200\n" */
/*                                                    "300\n" */
/*                                                    "500\n" */
/*                                                    "1000"); */
/*   lv_obj_align(step_number_roller, LV_ALIGN_LEFT_MID, 20, 0); */
/*   lv_roller_set_selected(step_number_roller, 6, LV_ANIM_OFF); */
/*   lv_obj_set_event_cb(step_number_roller, [](lv_obj_t *btn, lv_event_t event) { */
/*     static_view_pointer->event_cb(ACTION_SET_STEP_NUMBER, btn, event); */
/*   }); */
/* #endif */

/*     lv_obj_t *plan_container = */
/*         Display::add_container(parent, LV_HOR_RES / 2, 100); */
/*     lv_obj_align(plan_container, LV_ALIGN_TOP_RIGHT, 0, 0); */
/*     plan_label = Display::add_label(plan_container); */

/*     lv_obj_t *start_button = Display::add_button(parent, "start", 70, 70); */
/*     lv_obj_align(start_button, LV_ALIGN_BOTTOM_RIGHT, -70, -20); */
/*     // lv_obj_set_event_cb(start_button, [](lv_obj_t *btn, lv_event_t event) */
/*     //                     { static_view_pointer->event_cb(ACTION_START_STACK, btn, event); }); */

/*     lv_obj_t *stop_button = Display::add_button(parent, "stop", 50, 50); */
/*     lv_obj_align(stop_button, LV_ALIGN_BOTTOM_RIGHT, -20, -20); */
/*     // lv_obj_set_event_cb(stop_button, [](lv_obj_t *btn, lv_event_t event) */
/*     //                     { static_view_pointer->event_cb(ACTION_STOP_STACK, btn, event); }); */
/* } */

/* void View::fill_config_panel(lv_obj_t *parent) */
/* { */
/*     lv_obj_t *config_container = Display::add_container(parent, LV_HOR_RES / 2, 100); */
/*     lv_obj_align(config_container, LV_ALIGN_TOP_RIGHT, 0, 0); */
/*     // config_label = Display::add_label(config_container); */
/* } */

/* View::View(Model *_model, Controller *_controller) : Display(), model{_model}, controller{_controller} */
/* { */
/*     static_view_pointer = this; */

/*     move_tab = make_tab("move"); */
/*     fill_move_panel(move_tab); */
/*     stack_tab = make_tab("stack"); */
/*     /1* fill_stack_panel(stack_tab); *1/ */
/*     config_tab = make_tab("cfg"); */
/*     /1* fill_config_panel(config_tab); *1/ */
/* } */

/* int View::position_as_nm(const int pitch_per_rev, const int pulses_per_rev, const int position) { */
/*   return (position * pitch_per_rev * 1000) / pulses_per_rev; */

/* } */

/* /1* static float compute_position_in_mm(const struct stepper_status *status, int position){ *1/ */
/* /1*   return (float)position; // * (float)status->pitch_per_rev / (float)status->pulses_per_rev; *1/ */
/* /1* } *1/ */

/* /1* static char *render_position_in_mm(const struct stepper_status *status, int position) { *1/ */
/* /1*   char buf[1000]; *1/ */
/* /1*   snprintf(buf, sizeof(buf), "%lf mm", render_position_in_mm(status, position)); *1/ */
/* /1*   return buf; *1/ */
/* /1* } *1/ */

/* void View::update_status_label(const struct model_status status) */
/* { */
/*     char buf[1000]; */
/*     const struct stepper_with_target_status *stepper_with_target_status = &status.stepper_with_target_status; */
/*     const struct stepper_status *stepper_status = &stepper_with_target_status->stepper_status; */
/*     const int pitch_per_rev = stepper_status->pitch_per_rev; */
/*     const int pulses_per_rev = stepper_status->pulses_per_rev; */

/*     const int position_nm = position_as_nm(pitch_per_rev, pulses_per_rev, stepper_status->position); */

/*     if (stepper_with_target_status->is_moving) */
/*     { */
/*       const int target_position_nm = position_as_nm(pitch_per_rev, pulses_per_rev, stepper_with_target_status->target_position); */
/*         if (stepper_with_target_status->target_position > stepper_status->position) */
/*         { */
/*             snprintf(buf, sizeof(buf), "%d (-> %d)", position_nm, target_position_nm); */
/*         } */
/*         else */
/*         { */
/*             snprintf(buf, sizeof(buf), "(%d <-) %d", target_position_nm, position_nm); */
/*         } */
/*     } */
/*     else */
/*     { */
/*         snprintf(buf, sizeof(buf), "@%dnm", position_nm); */
/*     } */
/*     Display::set_status(buf); */

/*     const int lower_bound_nm = position_as_nm(pitch_per_rev, pulses_per_rev, status.lower_bound); */
/*     if (stepper_status->position > status.lower_bound) { */
/*       snprintf(buf, sizeof(buf), "%d <<", lower_bound_nm); */
/*     } else if (stepper_status->position == status.lower_bound) { */
/*       snprintf(buf, sizeof(buf), "%d ==", lower_bound_nm); */
/*     } else { */
/*       snprintf(buf, sizeof(buf), "%d >>", lower_bound_nm); */
/*     } */
/*     Display::set_status_left(buf); */

/*     const int upper_bound_nm = position_as_nm(pitch_per_rev, pulses_per_rev, status.upper_bound); */
/*     if (stepper_status->position < status.upper_bound) { */
/*       snprintf(buf, sizeof(buf), "<< %d", upper_bound_nm); */
/*     } else if (stepper_status->position == status.upper_bound) { */
/*       snprintf(buf, sizeof(buf), "%d ==", upper_bound_nm); */
/*     } else { */
/*       snprintf(buf, sizeof(buf), ">> %d", upper_bound_nm); */
/*     } */
/*     Display::set_status_right(buf); */
/* } */

/* void View::update() */
/* { */
/*     update_status_label(model->get_status()); */
/*     Display::run_task_handler(); */
/* } */

