#include "evw/gamefiles/hashsearch.h"

#include <atomic>
#include <thread>

#include "evw/gamefiles/sha1.h"

namespace evw::gamefiles
{
    namespace
    {
        constexpr size_t ALIGN_LENGTH = 8;
    }

    std::vector<std::vector<uint8_t>> hashSearch(const std::vector<uint8_t>& data,
                                                 const std::vector<std::array<uint8_t, 20>>& hashes,
                                                 int length)
    {
        std::vector<std::vector<uint8_t>> result(hashes.size());
        if (data.size() < static_cast<size_t>(length) || hashes.empty() || length <= 0)
            return result;

        const size_t lastStart = data.size() - static_cast<size_t>(length);
        const size_t positions = lastStart / ALIGN_LENGTH + 1;

        // Mark found hashes so threads can skip already-resolved targets.
        std::vector<std::atomic<bool>> found(hashes.size());
        for (auto& f : found) f.store(false);

        unsigned hw = std::thread::hardware_concurrency();
        unsigned nthreads = hw == 0 ? 1u : hw;
        if (positions < 4096) nthreads = 1; // not worth threading small inputs

        auto worker = [&](size_t startIdx, size_t endIdx) {
            for (size_t pi = startIdx; pi < endIdx; ++pi)
            {
                size_t pos = pi * ALIGN_LENGTH;
                auto h = evw::crypto::sha1(data.data() + pos, static_cast<size_t>(length));
                for (size_t j = 0; j < hashes.size(); ++j)
                {
                    if (found[j].load()) continue;
                    if (h == hashes[j])
                    {
                        result[j].assign(data.begin() + pos, data.begin() + pos + length);
                        found[j].store(true);
                    }
                }
            }
        };

        if (nthreads <= 1)
        {
            worker(0, positions);
        }
        else
        {
            std::vector<std::thread> pool;
            size_t per = (positions + nthreads - 1) / nthreads;
            for (unsigned t = 0; t < nthreads; ++t)
            {
                size_t s = t * per;
                size_t e = std::min(s + per, positions);
                if (s >= e) break;
                pool.emplace_back(worker, s, e);
            }
            for (auto& th : pool) th.join();
        }

        return result;
    }

    std::vector<uint8_t> hashSearchOne(const std::vector<uint8_t>& data,
                                       const std::array<uint8_t, 20>& hash, int length)
    {
        auto r = hashSearch(data, {hash}, length);
        return r.empty() ? std::vector<uint8_t>{} : r[0];
    }
}
