/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _FREERTOS_DRIVER_IMPL_H
#define _FREERTOS_DRIVER_IMPL_H

#include "driver.hpp"
#include <atomic>

namespace sys
{
class static_object : public virtual object
{
public:
    virtual void add_ref() override;
    virtual bool release() override;
};

class free_object_access : public virtual object_access
{
public:
    free_object_access() noexcept;

    virtual void open() override;
    virtual void close() override;

protected:
    virtual void on_first_open() = 0;
    virtual void on_last_close() = 0;

private:
    std::atomic<size_t> used_count_;
};

class semaphore_lock
{
public:
    semaphore_lock(SemaphoreHandle_t semaphore) noexcept;
    ~semaphore_lock();

private:
    SemaphoreHandle_t semaphore_;
};
}

#endif /* _FREERTOS_DRIVER_H */