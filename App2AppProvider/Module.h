#pragma once

/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025
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
 */

#ifndef MODULE_NAME
#define MODULE_NAME Plugin_App2AppProvider
#endif

// Core Thunder/WPEFramework plugin and interfaces
#include <plugins/plugins.h>

#if defined(__has_include)
#if __has_include(<interfaces/definitions.h>)
#include <interfaces/definitions.h>
#endif
#endif

#include <core/core.h>

#if defined(__has_include)
#if __has_include(<tracing/tracing.h>)
#include <tracing/tracing.h>
#else
// Fallback for test builds without Thunder tracing headers
#ifndef SYSLOG
#define SYSLOG(...) do { } while (0)
#endif
#endif
#else
// If __has_include is not available, provide a safe SYSLOG no-op
#ifndef SYSLOG
#define SYSLOG(...) do { } while (0)
#endif
#endif

#undef EXTERNAL
#define EXTERNAL
