#ifndef SCOPEMUX_SOURCE_RANGE_H
#define SCOPEMUX_SOURCE_RANGE_H

#include <stdint.h>

/**
 * @brief Representation of a source location
 */
typedef struct {
  uint32_t line;   // 0-indexed line number
  uint32_t column; // 0-indexed column number
  uint32_t offset; // Byte offset from the start of the file
} SourceLocation;

/**
 * @brief Representation of a source range
 */
typedef struct {
  SourceLocation start;
  SourceLocation end;
} SourceRange;

#endif /* SCOPEMUX_SOURCE_RANGE_H */
