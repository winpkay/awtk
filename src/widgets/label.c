﻿/**
 * File:   label.h
 * Author: AWTK Develop Team
 * Brief:  label
 *
 * Copyright (c) 2018 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2018-02-06 Li XianJing <xianjimli@hotmail.com> created
 * 2018-12-05 Li XianJing <xianjimli@hotmail.com> supported multi line
 *
 */

#include "tkc/mem.h"
#include "widgets/label.h"
#include "base/line_break.h"
#include "base/widget_vtable.h"
#include "base/window_manager.h"

typedef struct _label_line_parser_t {
  canvas_t* c;
  uint32_t size;
  uint32_t width;
  uint32_t font_size;
  const wchar_t* str;

  uint32_t total_lines;
  uint32_t total_paragraphs;

  /*current line*/
  const wchar_t* line;
  uint32_t line_size;
  bool_t is_force_break;
} label_line_parser_t;

static ret_t label_line_parser_next(label_line_parser_t* parser);
static ret_t label_line_parser_init(label_line_parser_t* parser, canvas_t* c, const wchar_t* str,
                                    uint32_t size, uint32_t font_size, uint32_t width) {
  return_value_if_fail(parser != NULL && str != NULL && size > 0, RET_BAD_PARAMS);
  return_value_if_fail(font_size > 0 && width > font_size, RET_BAD_PARAMS);

  memset(parser, 0x00, sizeof(label_line_parser_t));

  parser->c = c;
  parser->str = str;
  parser->size = size;
  parser->width = width;
  parser->font_size = font_size;

  parser->line = str;
  parser->line_size = 0;
  parser->is_force_break = FALSE;

  while (label_line_parser_next(parser) == RET_OK) {
    if (parser->is_force_break) {
      parser->total_paragraphs++;
    }
    parser->total_lines++;
  }

  if (!parser->is_force_break) {
    parser->total_paragraphs++;
  }

  parser->line = str;
  parser->line_size = 0;

  return RET_OK;
}

static ret_t label_line_parser_next(label_line_parser_t* parser) {
  int32_t w = 0;
  int32_t char_w = 0;
  const wchar_t* p = NULL;
  canvas_t* c = parser->c;
  return_value_if_fail(parser != NULL && parser->str != NULL, RET_BAD_PARAMS);

  parser->line = parser->line + parser->line_size;
  parser->line_size = 0;

  if ((parser->line - parser->str) >= parser->size) {
    return RET_EOS;
  }

  p = parser->line;
  parser->is_force_break = FALSE;
  while ((p - parser->str) < parser->size) {
    if (*p == '\r') {
      p++;
      if (*p == '\n') {
        p++;
      }
      parser->is_force_break = TRUE;
      break;
    } else if (*p == '\n') {
      p++;
      parser->is_force_break = TRUE;
      break;
    }

    char_w = canvas_measure_text(c, p, 1) + 1;
    if ((w + char_w) > parser->width) {
      if (line_break_check(p[0], p[1]) == LINE_BREAK_NO) {
        if (p > parser->line) {
          p--;
        }
      }
      break;
    }

    p++;
    w = canvas_measure_text(c, parser->line, p - parser->line);
  }

  parser->line_size = p - parser->line;

  return RET_OK;
}

static ret_t label_paint_text_mlines(widget_t* widget, canvas_t* c, label_line_parser_t* p) {
  int32_t x = 0;
  int32_t y = 0;
  int32_t w = 0;
  int32_t h_text = 0;
  style_t* style = widget->astyle;
  int32_t font_size = c->font_size;
  int32_t margin = style_get_int(style, STYLE_ID_MARGIN, 2);
  int32_t spacer = style_get_int(style, STYLE_ID_SPACER, 2);
  align_v_t align_v = (align_v_t)style_get_int(style, STYLE_ID_TEXT_ALIGN_V, ALIGN_V_TOP);
  align_h_t align_h = (align_h_t)style_get_int(style, STYLE_ID_TEXT_ALIGN_H, ALIGN_H_LEFT);
  int32_t line_height = font_size + spacer;

  x = margin;
  y = margin;
  w = widget->w - margin - margin;
  h_text = p->total_lines * line_height;

  switch (align_v) {
    case ALIGN_V_TOP: {
      y = margin;
      break;
    }
    case ALIGN_V_BOTTOM: {
      y = widget->h - margin - h_text;
      break;
    }
    default: {
      y = (widget->h - h_text) / 2;
      break;
    }
  }

  if (y < margin) {
    y = margin;
  }

  canvas_set_text_align(c, align_h, align_v);
  while (label_line_parser_next(p) == RET_OK) {
    uint32_t size = 0;
    rect_t r = rect_init(x, y, w, font_size);

    if ((y + font_size) > widget->h) {
      break;
    }

    for (size = 0; size < p->line_size; size++) {
      if (p->line[size] == '\r' || p->line[size] == '\n') {
        break;
      }
    }
    canvas_draw_text_in_rect(c, p->line, size, &r);

    y += line_height;
  }

  return RET_OK;
}

static ret_t label_paint_text(widget_t* widget, canvas_t* c, const wchar_t* str, uint32_t size) {
  label_line_parser_t p;
  style_t* style = widget->astyle;
  int32_t margin = style_get_int(style, STYLE_ID_MARGIN, 2);
  int32_t w = widget->w - margin - margin;

  return_value_if_fail(
      label_line_parser_init(&p, c, widget->text.str, widget->text.size, c->font_size, w) == RET_OK,
      RET_BAD_PARAMS);

  if (p.total_lines > 1) {
    return label_paint_text_mlines(widget, c, &p);
  } else {
    wstr_t str = widget->text;
    str.size = size;

    return widget_paint_helper(widget, c, NULL, &str);
  }
}

static ret_t label_on_paint_self(widget_t* widget, canvas_t* c) {
  if (widget->text.size > 0 && style_is_valid(widget->astyle)) {
    label_t* label = LABEL(widget);
    uint32_t size =
        label->length >= 0 ? tk_min(label->length, widget->text.size) : widget->text.size;

    widget_prepare_text_style(widget, c);

    label_paint_text(widget, c, widget->text.str, size);
  }

  return RET_OK;
}

ret_t label_resize_to_content(widget_t* widget, uint32_t min_w, uint32_t max_w, uint32_t min_h,
                              uint32_t max_h) {
  wh_t w = 0;
  wh_t h = 0;
  int32_t margin = 0;
  int32_t spacer = 0;
  int32_t line_height = 0;
  style_t* style = NULL;
  label_line_parser_t p;
  label_t* label = LABEL(widget);
  canvas_t* c = widget_get_canvas(widget);
  return_value_if_fail(label != NULL, RET_BAD_PARAMS);

  style = widget->astyle;
  margin = style_get_int(style, STYLE_ID_MARGIN, 2);
  spacer = style_get_int(style, STYLE_ID_SPACER, 2);
  line_height = c->font_size + spacer;

  widget_prepare_text_style(widget, c);
  w = canvas_measure_text(c, widget->text.str, widget->text.size) + 2 * margin;

  w = tk_clampi(w, min_w, max_w);
  return_value_if_fail(
      label_line_parser_init(&p, c, widget->text.str, widget->text.size, c->font_size, w) == RET_OK,
      RET_BAD_PARAMS);

  h = p.total_lines * line_height + 2 * margin;
  h = tk_clampi(h, min_h, max_h);

  widget_resize(widget, w, h);

  return RET_OK;
}

ret_t label_set_length(widget_t* widget, int32_t length) {
  label_t* label = LABEL(widget);
  return_value_if_fail(label != NULL, RET_BAD_PARAMS);
  label->length = length;

  return widget_invalidate_force(widget, NULL);
}

static ret_t label_get_prop(widget_t* widget, const char* name, value_t* v) {
  label_t* label = LABEL(widget);
  return_value_if_fail(label != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(name, WIDGET_PROP_VALUE)) {
    double d = 0;
    wstr_to_float(&(widget->text), &d);
    value_set_double(v, d);
    return RET_OK;
  } else if (tk_str_eq(name, WIDGET_PROP_LENGTH)) {
    value_set_int(v, label->length);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t label_set_prop(widget_t* widget, const char* name, const value_t* v) {
  return_value_if_fail(widget != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(name, WIDGET_PROP_VALUE)) {
    return wstr_from_value(&(widget->text), v);
  } else if (tk_str_eq(name, WIDGET_PROP_LENGTH)) {
    return label_set_length(widget, tk_roundi(value_float(v)));
  }

  return RET_NOT_FOUND;
}

static ret_t label_auto_adust_size(widget_t* widget) {
  wh_t w = 0;
  int32_t margin = 0;
  int32_t spacer = 0;
  int32_t line_height = 0;
  style_t* style = NULL;
  label_line_parser_t p;
  label_t* label = LABEL(widget);
  canvas_t* c = widget_get_canvas(widget);
  return_value_if_fail(label != NULL && c != NULL && widget->astyle != NULL, RET_BAD_PARAMS);

  style = widget->astyle;
  margin = style_get_int(style, STYLE_ID_MARGIN, 2);
  spacer = style_get_int(style, STYLE_ID_SPACER, 2);
  line_height = c->font_size + spacer;

  w = widget->w - 2 * margin;
  widget_prepare_text_style(widget, c);
  return_value_if_fail(
      label_line_parser_init(&p, c, widget->text.str, widget->text.size, c->font_size, w) == RET_OK,
      RET_BAD_PARAMS);

  widget->h = line_height * p.total_lines;

  return RET_OK;
}

static ret_t label_on_event(widget_t* widget, event_t* e) {
  uint16_t type = e->type;
  label_t* label = LABEL(widget);
  return_value_if_fail(label != NULL && widget != NULL, RET_BAD_PARAMS);

  switch (type) {
    case EVT_RESIZE:
    case EVT_MOVE_RESIZE: {
      if (widget->auto_adjust_size) {
        label_auto_adust_size(widget);
        break;
      }
      break;
    }
    default:
      break;
  }

  return RET_OK;
}

static const char* const s_label_properties[] = {WIDGET_PROP_LENGTH, NULL};

TK_DECL_VTABLE(label) = {.size = sizeof(label_t),
                         .type = WIDGET_TYPE_LABEL,
                         .parent = TK_PARENT_VTABLE(widget),
                         .clone_properties = s_label_properties,
                         .persistent_properties = s_label_properties,
                         .create = label_create,
                         .set_prop = label_set_prop,
                         .get_prop = label_get_prop,
                         .on_event = label_on_event,
                         .on_paint_self = label_on_paint_self};

widget_t* label_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  widget_t* widget = widget_create(parent, TK_REF_VTABLE(label), x, y, w, h);
  label_t* label = LABEL(widget);
  return_value_if_fail(label != NULL, NULL);

  label->length = -1;

  return widget;
}

widget_t* label_cast(widget_t* widget) {
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, label), NULL);

  return widget;
}
