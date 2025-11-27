#pragma once

#include "Models.h"
#include <filesystem>
#include <string>

namespace engine {

    class EvaluationService {
    public:
        EvaluationService(std::filesystem::path baseDir,
                          std::string dockerImage);

        EvaluationResult evaluate(const SubmissionRequest& request);

    private:
        std::filesystem::path baseDir_;
        std::string dockerImage_;
    };

} // namespace engine
