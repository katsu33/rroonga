/* -*- c-file-style: "ruby" -*- */
/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "rb-grn.h"

#define SELF(object) (rb_rb_grn_snippet_from_ruby_object(object))

typedef struct _RbGrnSnippet RbGrnSnippet;
struct _RbGrnSnippet
{
    grn_ctx *context;
    grn_snip *snippet;
    rb_grn_boolean owner;
};

VALUE rb_cGrnSnippet;

static RbGrnSnippet *
rb_rb_grn_snippet_from_ruby_object (VALUE object)
{
    RbGrnSnippet *rb_grn_snippet;

    if (!RVAL2CBOOL(rb_obj_is_kind_of(object, rb_cGrnSnippet))) {
	rb_raise(rb_eTypeError, "not a groonga snippet");
    }

    Data_Get_Struct(object, RbGrnSnippet, rb_grn_snippet);
    if (!rb_grn_snippet)
	rb_raise(rb_eGrnError, "groonga snippet is NULL");

    return rb_grn_snippet;
}

grn_snip *
rb_grn_snippet_from_ruby_object (VALUE object)
{
    if (NIL_P(object))
        return NULL;

    return SELF(object)->snippet;
}

static void
rb_rb_grn_snippet_free (void *object)
{
    RbGrnSnippet *rb_grn_snippet = object;

    if (rb_grn_snippet->owner &&
	rb_grn_snippet->context && rb_grn_snippet->snippet)
        grn_snip_close(rb_grn_snippet->context,
                       rb_grn_snippet->snippet);

    xfree(object);
}

VALUE
rb_grn_snippet_to_ruby_object (grn_ctx *context, grn_snip *snippet,
			       rb_grn_boolean owner)
{
    RbGrnSnippet *rb_grn_snippet;

    if (!snippet)
        return Qnil;

    rb_grn_snippet = ALLOC(RbGrnSnippet);
    rb_grn_snippet->context = context;
    rb_grn_snippet->snippet = snippet;
    rb_grn_snippet->owner = owner;

    return Data_Wrap_Struct(rb_cGrnSnippet, NULL,
                            rb_rb_grn_snippet_free, rb_grn_snippet);
}

static VALUE
rb_grn_snippet_alloc (VALUE klass)
{
    return Data_Wrap_Struct(klass, NULL, rb_rb_grn_snippet_free, NULL);
}

static VALUE
rb_grn_snippet_initialize (int argc, VALUE *argv, VALUE self)
{
    RbGrnSnippet *rb_grn_snippet;
    grn_ctx *context = NULL;
    grn_snip *snippet = NULL;
    VALUE options;
    VALUE rb_context, rb_normalize, rb_skip_leading_spaces;
    VALUE rb_width, rb_max_results, rb_default_open_tag, rb_default_close_tag;
    VALUE rb_html_escape;
    int flags = GRN_SNIP_COPY_TAG;
    unsigned int width = 100;
    unsigned int max_results = 3;
    char *default_open_tag = NULL;
    unsigned int default_open_tag_length = 0;
    char *default_close_tag = NULL;
    unsigned int default_close_tag_length = 0;
    grn_snip_mapping *mapping = NULL;

    rb_scan_args(argc, argv, "01", &options);

    rb_grn_scan_options(options,
                        "context", &rb_context,
                        "normalize", &rb_normalize,
                        "skip_leading_spaces", &rb_skip_leading_spaces,
                        "width", &rb_width,
                        "max_results", &rb_max_results,
                        "default_open_tag", &rb_default_open_tag,
                        "default_close_tag", &rb_default_close_tag,
                        "html_escape", &rb_html_escape,
                        NULL);

    context = rb_grn_context_ensure(&rb_context);

    if (RVAL2CBOOL(rb_normalize))
        flags |= GRN_SNIP_NORMALIZE;
    if (RVAL2CBOOL(rb_skip_leading_spaces))
        flags |= GRN_SNIP_SKIP_LEADING_SPACES;

    if (!NIL_P(rb_width))
        width = NUM2UINT(rb_width);

    if (!NIL_P(rb_max_results))
        max_results = NUM2UINT(rb_max_results);

    if (!NIL_P(rb_default_open_tag)) {
        default_open_tag = StringValuePtr(rb_default_open_tag);
        default_open_tag_length = RSTRING_LEN(rb_default_open_tag);
    }

    if (!NIL_P(rb_default_close_tag)) {
        default_close_tag = StringValuePtr(rb_default_close_tag);
        default_close_tag_length = RSTRING_LEN(rb_default_close_tag);
    }

    if (RVAL2CBOOL(rb_html_escape))
        mapping = (grn_snip_mapping *)-1;

    snippet = grn_snip_open(context, flags, width, max_results,
                            default_open_tag, default_open_tag_length,
                            default_close_tag, default_close_tag_length,
                            mapping);
    rb_grn_context_check(context, rb_ary_new4(argc, argv));

    rb_grn_snippet = ALLOC(RbGrnSnippet);
    DATA_PTR(self) = rb_grn_snippet;
    rb_grn_snippet->context = context;
    rb_grn_snippet->snippet = snippet;
    rb_grn_snippet->owner = RB_GRN_TRUE;

    rb_iv_set(self, "context", rb_context);

    return Qnil;
}

static VALUE
rb_grn_snippet_add_keyword (int argc, VALUE *argv, VALUE self)
{
    RbGrnSnippet *rb_grn_snippet;
    grn_rc rc;
    VALUE rb_keyword, options;
    VALUE rb_open_tag, rb_close_tag;
    char *keyword, *open_tag = NULL, *close_tag = NULL;
    unsigned int keyword_length, open_tag_length = 0, close_tag_length = 0;

    rb_scan_args(argc, argv, "11", &rb_keyword, &options);

    rb_grn_snippet = SELF(self);

    keyword = StringValuePtr(rb_keyword);
    keyword_length = RSTRING_LEN(rb_keyword);

    rb_grn_scan_options(options,
                        "open_tag", &rb_open_tag,
                        "close_tag", &rb_close_tag,
                        NULL);

    if (!NIL_P(rb_open_tag)) {
        open_tag = StringValuePtr(rb_open_tag);
        open_tag_length = RSTRING_LEN(rb_open_tag);
    }

    if (!NIL_P(rb_close_tag)) {
        close_tag = StringValuePtr(rb_close_tag);
        close_tag_length = RSTRING_LEN(rb_close_tag);
    }

    rc = grn_snip_add_cond(rb_grn_snippet->context,
                           rb_grn_snippet->snippet,
                           keyword, keyword_length,
                           open_tag, open_tag_length,
                           close_tag, close_tag_length);
    rb_grn_rc_check(rc, self);

    return Qnil;
}

static VALUE
rb_grn_snippet_execute (VALUE self, VALUE rb_string)
{
    RbGrnSnippet *rb_grn_snippet;
    grn_rc rc;
    grn_ctx *context;
    grn_snip *snippet;
    char *string;
    unsigned int string_length;
    unsigned int i, n_results, max_tagged_length;
    VALUE rb_results;
    char *result;

    rb_grn_snippet = SELF(self);
    context = rb_grn_snippet->context;
    snippet = rb_grn_snippet->snippet;

#ifdef HAVE_RUBY_ENCODING_H
    rb_string = rb_grn_context_rb_string_encode(context, rb_string);
#endif
    string = StringValuePtr(rb_string);
    string_length = RSTRING_LEN(rb_string);

    rc = grn_snip_exec(context, snippet, string, string_length,
                       &n_results, &max_tagged_length);
    rb_grn_rc_check(rc, self);

    rb_results = rb_ary_new2(n_results);
    result = ALLOCA_N(char, max_tagged_length);
    for (i = 0; i < n_results; i++) {
        VALUE rb_result;
        unsigned result_length;

        rc = grn_snip_get_result(context, snippet, i, result, &result_length);
        rb_grn_rc_check(rc, self);
        rb_result = rb_grn_context_rb_string_new(context, result, result_length);
        rb_ary_push(rb_results, rb_result);
    }

    return rb_results;
}

void
rb_grn_init_snippet (VALUE mGrn)
{
    rb_cGrnSnippet = rb_define_class_under(mGrn, "Snippet", rb_cObject);
    rb_define_alloc_func(rb_cGrnSnippet, rb_grn_snippet_alloc);

    rb_define_method(rb_cGrnSnippet, "initialize",
                     rb_grn_snippet_initialize, -1);
    rb_define_method(rb_cGrnSnippet, "add_keyword",
                     rb_grn_snippet_add_keyword, -1);
    rb_define_method(rb_cGrnSnippet, "execute",
                     rb_grn_snippet_execute, 1);
}
