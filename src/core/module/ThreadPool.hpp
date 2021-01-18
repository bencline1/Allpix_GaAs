/**
 * @file
 * @brief Definition of thread pool for concurrent events
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_THREADPOOL_H
#define ALLPIX_THREADPOOL_H

#include <algorithm>
#include <atomic>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <thread>
#include <utility>

namespace allpix {
    /**
     * @brief Pool of threads where event tasks can be submitted to
     */
    class ThreadPool {
    public:
        /**
         * @brief Internal thread-safe queue
         */
        template <typename T> class SafeQueue {
        public:
            /**
             * @brief Default constructor, initializes empty queue
             */
            explicit SafeQueue(unsigned int max_size);

            /**
             * @brief Erases the queue and release waiting threads on destruction
             */
            ~SafeQueue();

            /**
             * @brief Get the top value in the safe queue
             * @param out Reference where the value at the top of the queue will be written to
             * @param wait If the method should wait for new tasks or should exit (defaults to wait)
             * @param func Optional function to execute before releasing the queue mutex if pop was successful
             * @return True if a task was acquired or false if pop was exited for another reason
             */
            bool pop(T& out, bool wait = true, const std::function<void()>& func = nullptr);

            /**
             * @brief Push a new value onto the safe queue
             * @param value Value to push to the queue
             */
            void push(T value);

            /**
             * @brief Return if the queue is in a valid state
             * @return True if the queue is valid, false if \ref SafeQueue::invalidate has been called
             */
            bool isValid() const;

            /**
             * @brief Return if the queue is empty or not
             * @return True if the queue is empty, false otherwise
             */
            bool empty() const;

            /**
             * @brief Return size of internal queue
             * @return Size of the size of the internal queue
             */
            size_t size() const;

            /**
             * @brief Invalidate the queue
             */
            void invalidate();

        private:
            std::atomic_bool valid_{true};
            mutable std::mutex mutex_;
            std::queue<T> queue_;
            std::condition_variable push_condition_;
            std::condition_variable pop_condition_;
            const unsigned int max_size_;
        };

        /**
         * @brief Construct thread pool with provided number of threads
         * @param num_threads Number of threads in the pool
         * @param max_queue_size Maximum size of the task queue
         * @param worker_init_function Function run by all the workers to initialize
         * @param worker_finalize_function Function run by all the workers to cleanup
         */
        explicit ThreadPool(unsigned int num_threads,
                            unsigned int max_queue_size,
                            const std::function<void()>& worker_init_function = nullptr,
                            const std::function<void()>& worker_finalize_function = nullptr);

        /// @{
        /**
         * @brief Copying the thread pool is not allowed
         */
        ThreadPool(const ThreadPool& rhs) = delete;
        ThreadPool& operator=(const ThreadPool& rhs) = delete;
        /// @}

        /**
         * @brief Submit a job to be run by the thread pool. In case no workers, the function will be immediately executed.
         * @param func Function to execute by the pool
         * @param args Parameters to pass to the function
         * @warning The thread submitting task should always call the \ref ThreadPool::execute method to prevent a lock when
         *          there are no threads available
         */
        template <typename Func, typename... Args> auto submit(Func&& func, Args&&... args);

        /**
         * @brief Return the number of enqueued events
         * @return The number of enqueued events
         */
        size_t queueSize() const;

        /**
         * @brief Check if any worker thread has thrown an exception
         * @throw Exception thrown by worker thread, if any
         */
        void checkException();

        /**
         * @brief Waits for the worker threads to finish
         */
        void wait();

        /**
         * @brief Invalidate all queues and joins all running threads when the pool is destroyed.
         */
        void destroy();

        /**
         * @brief Destroy and wait for all threads to finish on destruction
         */
        ~ThreadPool();

    private:
        /**
         * @brief Constantly running internal function each thread uses to acquire work items from the queue.
         * @param init_function Function to initialize the thread
         * @param init_function Function to finalize the thread
         */
        void worker(const std::function<void()>& init_function, const std::function<void()>& finalize_function);

        // The queue holds the task functions to be executed by the workers
        using Task = std::unique_ptr<std::packaged_task<void()>>;
        SafeQueue<Task> queue_;

        std::atomic_bool done_{false};

        std::atomic<unsigned int> run_cnt_{0};
        mutable std::mutex run_mutex_;
        std::condition_variable run_condition_;
        std::vector<std::thread> threads_;

        std::atomic_flag has_exception_{false};
        std::exception_ptr exception_ptr_{nullptr};
    };
} // namespace allpix

// Include template members
#include "ThreadPool.tpp"

#endif /* ALLPIX_THREADPOOL_H */
