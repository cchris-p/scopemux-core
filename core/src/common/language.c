/**
 * @file language.c
 * @brief Language detection and utilities implementation
 */

#include "scopemux/language.h"
#include <string.h>
#include <strings.h>

const char *language_to_string(Language lang) {
    switch (lang) {
        case LANG_C: return "c";
        case LANG_CPP: return "cpp";
        case LANG_PYTHON: return "python";
        case LANG_JAVASCRIPT: return "javascript";
        case LANG_TYPESCRIPT: return "typescript";
        case LANG_UNKNOWN:
        default: return "unknown";
    }
}

Language language_from_string(const char *lang_str) {
    if (!lang_str) return LANG_UNKNOWN;
    
    if (strcasecmp(lang_str, "c") == 0) return LANG_C;
    if (strcasecmp(lang_str, "cpp") == 0 || strcasecmp(lang_str, "c++") == 0) return LANG_CPP;
    if (strcasecmp(lang_str, "python") == 0 || strcasecmp(lang_str, "py") == 0) return LANG_PYTHON;
    if (strcasecmp(lang_str, "javascript") == 0 || strcasecmp(lang_str, "js") == 0) return LANG_JAVASCRIPT;
    if (strcasecmp(lang_str, "typescript") == 0 || strcasecmp(lang_str, "ts") == 0) return LANG_TYPESCRIPT;
    
    return LANG_UNKNOWN;
}

Language language_detect_from_extension(const char *file_path) {
    if (!file_path) return LANG_UNKNOWN;
    
    // Find the last dot in the filename
    const char *ext = strrchr(file_path, '.');
    if (!ext || ext == file_path) return LANG_UNKNOWN;
    
    ext++; // Skip the dot
    
    if (strcmp(ext, "c") == 0) return LANG_C;
    if (strcmp(ext, "h") == 0) return LANG_C;
    if (strcmp(ext, "cpp") == 0 || strcmp(ext, "cxx") == 0 || 
        strcmp(ext, "cc") == 0 || strcmp(ext, "C") == 0) return LANG_CPP;
    if (strcmp(ext, "hpp") == 0 || strcmp(ext, "hxx") == 0 || 
        strcmp(ext, "hh") == 0) return LANG_CPP;
    if (strcmp(ext, "py") == 0) return LANG_PYTHON;
    if (strcmp(ext, "js") == 0) return LANG_JAVASCRIPT;
    if (strcmp(ext, "ts") == 0) return LANG_TYPESCRIPT;
    
    return LANG_UNKNOWN;
}

const char *language_get_extension(Language lang) {
    switch (lang) {
        case LANG_C: return "c";
        case LANG_CPP: return "cpp";
        case LANG_PYTHON: return "py";
        case LANG_JAVASCRIPT: return "js";
        case LANG_TYPESCRIPT: return "ts";
        case LANG_UNKNOWN:
        default: return NULL;
    }
}

bool language_supports_interfile_references(Language lang) {
    switch (lang) {
        case LANG_C:
        case LANG_CPP:
        case LANG_PYTHON:
            return true;
        case LANG_JAVASCRIPT:
        case LANG_TYPESCRIPT:
            return true;
        case LANG_UNKNOWN:
        default:
            return false;
    }
}
