/* jq's match/test/sub/capture/scan/split on GFOpenRegex (regex.library,
 * through libregex.a) in Oniguruma's place: Perl syntax, UTF-8, the
 * captures' names read from the pattern (rx_names.c). Included by
 * src/builtin.c (patches/0001) when HAVE_GFOPENREGEX is defined.
 *
 * Modifiers as upstream's: g all matches, i any case, x extended (Perl's
 * (?x)), m and p "." takes a newline too (Oniguruma's MULTILINE is Perl's
 * s), s single line (already so: ^ and $ are the string's ends, as with
 * Oniguruma's Perl_NT syntax), n no empty matches. l (longest) is taken
 * but matching stays Perl's leftmost-first. Not offered by regex.library:
 * look-behind, possessive and atomic groups, \p{..} (a "Regex failure").
 *
 * MIT, Copyright (c) 2026 AmiCradle Team (jq is MIT too). */
#include <regex.h>
#include "rx_names.h"

/* code points before byte offset off */
static unsigned long gfrx_cp_index(const char *s, long off)
{
  unsigned long idx = 0;
  const char *fr = s;
  while (fr < s + off) {
    fr += jvp_utf8_decode_length(*fr);
    idx++;
  }
  return idx;
}

static jv gfrx_capture(const char *s, const regmatch_t *m, char *name)
{
  jv cap;
  if (m->rm_so < 0) {
    cap = jv_object_set(jv_object(), jv_string("offset"), jv_number(-1));
    cap = jv_object_set(cap, jv_string("length"), jv_number(0));
    cap = jv_object_set(cap, jv_string("string"), jv_null());
  } else {
    unsigned long idx = gfrx_cp_index(s, m->rm_so);
    unsigned long len = gfrx_cp_index(s + m->rm_so, m->rm_eo - m->rm_so);
    cap = jv_object_set(jv_object(), jv_string("offset"), jv_number(idx));
    cap = jv_object_set(cap, jv_string("length"), jv_number(len));
    cap = jv_object_set(cap, jv_string("string"),
                        jv_string_sized(s + m->rm_so, m->rm_eo - m->rm_so));
  }
  return jv_object_set(cap, jv_string("name"), name ? jv_string(name) : jv_null());
}

static jv f_match(jq_state *jq, jv input, jv regex, jv modifiers, jv testmode) {
  int test = jv_equal(testmode, jv_true());
  int global = 0, extended = 0, notempty = 0;
  int cflags = REG_PERL | REG_UTF8;
  jv result;

  if (jv_get_kind(input) != JV_KIND_STRING) {
    jv_free(regex);
    jv_free(modifiers);
    return type_error(input, "cannot be matched, as it is not a string");
  }
  if (jv_get_kind(regex) != JV_KIND_STRING) {
    jv_free(input);
    jv_free(modifiers);
    return type_error(regex, "is not a string");
  }
  if (jv_get_kind(modifiers) == JV_KIND_STRING) {
    jv modarray = jv_string_explode(jv_copy(modifiers));
    jv_array_foreach(modarray, i, mod) {
      switch ((int)jv_number_value(mod)) {
        case 'g': global = 1; break;
        case 'i': cflags |= REG_ICASE; break;
        case 'x': extended = 1; break;
        case 'm': case 'p': cflags |= REG_DOTALL; break;
        case 's': case 'l': break;
        case 'n': notempty = 1; break;
        default:
          jv_free(input);
          jv_free(regex);
          jv_free(modarray);
          return jv_invalid_with_msg(jv_string_concat(modifiers,
                jv_string(" is not a valid modifier string")));
      }
    }
    jv_free(modarray);
  } else if (jv_get_kind(modifiers) != JV_KIND_NULL) {
    jv_free(input);
    jv_free(regex);
    return type_error(modifiers, "is not a string");
  }
  jv_free(modifiers);

  /* x: Perl's (?x) in front of the pattern */
  jv pat = extended ? jv_string_concat(jv_string("(?x)"), jv_copy(regex)) : jv_copy(regex);
  regex_t re;
  int rc = regncomp(&re, jv_string_value(pat), jv_string_length_bytes(jv_copy(pat)), cflags);
  jv_free(pat);
  if (rc != 0) {
    char ebuf[160];
    regerror(rc, &re, ebuf, sizeof ebuf);
    jv_free(input);
    jv_free(regex);
    return jv_invalid_with_msg(jv_string_concat(jv_string("Regex failure: "), jv_string(ebuf)));
  }
  char **names = 0;
  int nnames = rx_group_names(jv_string_value(regex), jv_string_length_bytes(jv_copy(regex)),
                              extended, &names);
  if (nnames < 0) nnames = 0;
  size_t nm = re.re_nsub + 1;
  regmatch_t *m = jv_mem_alloc(nm * sizeof *m);

  const char *s = jv_string_value(input);
  long length = jv_string_length_bytes(jv_copy(input));
  long start = 0;
  result = test ? jv_false() : jv_array();
  do {
    m[0].rm_so = start;
    m[0].rm_eo = length;
    rc = regnexec(&re, s, length, nm, m, REG_STARTEND | (start > 0 ? REG_NOTBOL : 0));
    if (rc == REG_NOMATCH) break;
    if (rc != 0) {
      char ebuf[160];
      regerror(rc, &re, ebuf, sizeof ebuf);
      jv_free(result);
      result = jv_invalid_with_msg(jv_string_concat(jv_string("Regex failure: "), jv_string(ebuf)));
      break;
    }
    if (notempty && m[0].rm_eo == m[0].rm_so) {
      /* n: try again one character on */
      if (m[0].rm_so >= length) break;
      start = m[0].rm_so + jvp_utf8_decode_length(s[m[0].rm_so]);
      continue;
    }
    if (test) {
      result = jv_true();
      break;
    }
    jv match = jv_object_set(jv_object(), jv_string("offset"), jv_number(gfrx_cp_index(s, m[0].rm_so)));
    match = jv_object_set(match, jv_string("length"),
                          jv_number(gfrx_cp_index(s + m[0].rm_so, m[0].rm_eo - m[0].rm_so)));
    match = jv_object_set(match, jv_string("string"),
                          jv_string_sized(s + m[0].rm_so, m[0].rm_eo - m[0].rm_so));
    jv captures = jv_array();
    for (size_t i = 1; i < nm; ++i) {
      regmatch_t c = m[i];
      /* upstream: a group in an empty match reports the match's offset */
      if (m[0].rm_eo == m[0].rm_so && c.rm_so >= 0) c.rm_so = c.rm_eo = m[0].rm_so;
      captures = jv_array_append(captures,
          gfrx_capture(s, &c, (int)i <= nnames ? names[i - 1] : NULL));
    }
    match = jv_object_set(match, jv_string("captures"), captures);
    result = jv_array_append(result, match);
    if (m[0].rm_eo == m[0].rm_so) {
      /* an empty match: the next search starts one character on, so
       * '"qux" | match("(?=u)"; "g")' matches just once */
      if (m[0].rm_eo >= length) break;
      start = m[0].rm_eo + jvp_utf8_decode_length(s[m[0].rm_eo]);
    } else {
      start = m[0].rm_eo;
    }
  } while (global && start <= length);

  jv_mem_free(m);
  rx_names_free(names, nnames);
  regfree(&re);
  jv_free(input);
  jv_free(regex);
  return result;
}
