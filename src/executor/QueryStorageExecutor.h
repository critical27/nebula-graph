/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef EXECUTOR_QUERYSTORAGEEXECUTOR_H_
#define EXECUTOR_QUERYSTORAGEEXECUTOR_H_

#include "executor/Executor.h"
#include "common/clients/storage/StorageClientBase.h"

namespace nebula {
namespace graph {

// It's used for data write/update/query
class QueryStorageExecutor : public Executor {
protected:
    QueryStorageExecutor(const std::string &name, const PlanNode *node, QueryContext *qctx)
        : Executor(name, node, qctx) {}

    // parameter isCompleteRequire to specify is return error when partial succeeded
    template <typename Resp>
    StatusOr<Result::State>
    handleCompleteness(const storage::StorageRpcResponse<Resp> &rpcResp,
                       bool isCompleteRequire) const {
        auto completeness = rpcResp.completeness();
        // TODO(shylock) Maybe add option to treat the partial failed as error
        if (completeness != 100) {
            const auto &failedCodes = rpcResp.failedParts();
            for (auto it = failedCodes.begin(); it != failedCodes.end(); it++) {
                LOG(ERROR) << name_ << " failed, error "
                           << storage::cpp2::_ErrorCode_VALUES_TO_NAMES.at(it->second) << ", part "
                           << it->first;
            }
            if (completeness == 0 || isCompleteRequire) {
                return handleErrorCode(failedCodes.begin()->second,
                                       failedCodes.begin()->first);
            }
            return Result::State::kPartialSuccess;
        }
        return Result::State::kSuccess;
    }

    Status handleErrorCode(nebula::storage::cpp2::ErrorCode code, PartitionID partId) const {
        switch (code) {
            case storage::cpp2::ErrorCode::E_INVALID_VID:
                return Status::Error("Storage Error: Invalid vid.");
            case storage::cpp2::ErrorCode::E_INVALID_FIELD_VALUE: {
                std::string error = "Storage Error: Invalid field value: "
                                    "may be the filed is not NULL "
                                    "or without default value or wrong schema.";
                return Status::Error(std::move(error));
            }
            case storage::cpp2::ErrorCode::E_LEADER_CHANGED:
                return Status::Error("Storage Error: Leader changed.");
            case storage::cpp2::ErrorCode::E_INVALID_FILTER:
                return Status::Error("Storage Error: Invalid filter.");
            case storage::cpp2::ErrorCode::E_INVALID_UPDATER:
                return Status::Error("Storage Error: Invalid Update col or yield col.");
            case storage::cpp2::ErrorCode::E_INVALID_SPACEVIDLEN:
                return Status::Error("Storage Error: Invalid space vid len.");
            case storage::cpp2::ErrorCode::E_SPACE_NOT_FOUND:
                return Status::Error("Storage Error: Space not found.");
            case storage::cpp2::ErrorCode::E_TAG_NOT_FOUND:
                return Status::Error("Storage Error: Tag not found.");
            case storage::cpp2::ErrorCode::E_TAG_PROP_NOT_FOUND:
                return Status::Error("Storage Error: Tag prop not found.");
            case storage::cpp2::ErrorCode::E_EDGE_NOT_FOUND:
                return Status::Error("Storage Error: Edge not found.");
            case storage::cpp2::ErrorCode::E_EDGE_PROP_NOT_FOUND:
                return Status::Error("Storage Error: Edge prop not found.");
            case storage::cpp2::ErrorCode::E_INDEX_NOT_FOUND:
                return Status::Error("Storage Error: Index not found.");
            case storage::cpp2::ErrorCode::E_INVALID_DATA:
                return Status::Error("Storage Error: Invalid data, may be wrong value type.");
            case storage::cpp2::ErrorCode::E_NOT_NULLABLE:
                return Status::Error("Storage Error: The not null field cannot be null.");
            case storage::cpp2::ErrorCode::E_FIELD_UNSET:
                return Status::Error("Storage Error: "
                                     "The not null field doesn't have a default value.");
            case storage::cpp2::ErrorCode::E_OUT_OF_RANGE:
                return Status::Error("Storage Error: Out of range value.");
            case storage::cpp2::ErrorCode::E_ATOMIC_OP_FAILED:
                return Status::Error("Storage Error: Atomic operation failed.");
            case storage::cpp2::ErrorCode::E_FILTER_OUT:
                return Status::OK();
            default:
                auto status = Status::Error("Storage Error: part: %d, error code: %d.",
                                             partId, static_cast<int32_t>(code));
                LOG(ERROR) << status;
                return status;
        }
        return Status::OK();
    }
};

}   // namespace graph
}   // namespace nebula

#endif  // EXECUTOR_QUERYSTORAGEEXECUTOR_H_
