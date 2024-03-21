#pragma once

#include "vulkan_types.inl"

// returns the string representation of result
// @param result the result to get the string for
//@param get_extended indicates whether to also return an extended result
// @returns the error code and/or extended error message in string form. defaults to success for unknown result types
const char* vulkan_result_string(VkResult result, b8 get_extended);  // give back the results in astring format for messanging purposes, set extened true for more verbose - use for debugging stuffs

// indicates if the passed result is a success or an error as defined by the vulkan spec
// @returns true if success; otherwise false. defaults to true for unknown result types
b8 vulkan_result_is_success(VkResult result);  // check whether the vulkan results are successful pass in only the results