/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "executor/maintain/EdgeIndexExecutor.h"
#include "planner/Maintain.h"
#include "util/IndexUtil.h"

namespace nebula {
namespace graph {

folly::Future<Status> CreateEdgeIndexExecutor::execute() {
    SCOPED_TIMER(&execTime_);

    auto *ceiNode = asNode<CreateEdgeIndex>(node());
    auto spaceId = qctx()->rctx()->session()->space().id;
    return qctx()
        ->getMetaClient()
        ->createEdgeIndex(spaceId,
                          ceiNode->getIndexName(),
                          ceiNode->getSchemaName(),
                          ceiNode->getFields(),
                          ceiNode->getIfNotExists())
        .via(runner())
        .then([ceiNode, spaceId](StatusOr<IndexID> resp) {
            if (!resp.ok()) {
                LOG(ERROR) << "SpaceId: " << spaceId << ", Create index `"
                           << ceiNode->getIndexName() << "' at edge: `" << ceiNode->getSchemaName()
                           << "' failed: " << resp.status();
                return resp.status();
            }
            return Status::OK();
        });
}

folly::Future<Status> DropEdgeIndexExecutor::execute() {
    SCOPED_TIMER(&execTime_);

    auto *deiNode = asNode<DropEdgeIndex>(node());
    auto spaceId = qctx()->rctx()->session()->space().id;
    return qctx()
        ->getMetaClient()
        ->dropEdgeIndex(spaceId, deiNode->getIndexName(), deiNode->getIfExists())
        .via(runner())
        .then([deiNode, spaceId](StatusOr<IndexID> resp) {
            if (!resp.ok()) {
                LOG(ERROR) << "SpaceId: " << spaceId << ", Drop edge index`"
                           << deiNode->getIndexName() << "' failed: " << resp.status();
                return resp.status();
            }
            return Status::OK();
        });
}

folly::Future<Status> DescEdgeIndexExecutor::execute() {
    SCOPED_TIMER(&execTime_);

    auto *deiNode = asNode<DescEdgeIndex>(node());
    auto spaceId = qctx()->rctx()->session()->space().id;
    return qctx()
        ->getMetaClient()
        ->getEdgeIndex(spaceId, deiNode->getIndexName())
        .via(runner())
        .then([this, deiNode, spaceId](StatusOr<meta::cpp2::IndexItem> resp) {
            if (!resp.ok()) {
                LOG(ERROR) << "SpaceId: " << spaceId << ", Desc edge index`"
                           << deiNode->getIndexName() << "' failed: " << resp.status();
                return resp.status();
            }

            auto ret = IndexUtil::toDescIndex(resp.value());
            if (!ret.ok()) {
                LOG(ERROR) << ret.status();
                return ret.status();
            }
            return finish(ResultBuilder()
                              .value(std::move(ret).value())
                              .iter(Iterator::Kind::kDefault)
                              .finish());
        });
}

folly::Future<Status> ShowCreateEdgeIndexExecutor::execute() {
    SCOPED_TIMER(&execTime_);

    auto *sceiNode = asNode<ShowCreateEdgeIndex>(node());
    auto spaceId = qctx()->rctx()->session()->space().id;
    return qctx()
        ->getMetaClient()
        ->getEdgeIndex(spaceId, sceiNode->getIndexName())
        .via(runner())
        .then([this, sceiNode, spaceId](StatusOr<meta::cpp2::IndexItem> resp) {
            if (!resp.ok()) {
                LOG(ERROR) << "SpaceId: " << spaceId << ", Show create edge index `"
                           << sceiNode->getIndexName() << "' failed: " << resp.status();
                return resp.status();
            }
            auto ret = IndexUtil::toShowCreateIndex(false, sceiNode->getIndexName(), resp.value());
            if (!ret.ok()) {
                LOG(ERROR) << ret.status();
                return ret.status();
            }
            return finish(ResultBuilder()
                              .value(std::move(ret).value())
                              .iter(Iterator::Kind::kDefault)
                              .finish());
        });
}

folly::Future<Status> ShowEdgeIndexesExecutor::execute() {
    SCOPED_TIMER(&execTime_);

    auto spaceId = qctx()->rctx()->session()->space().id;
    return qctx()->getMetaClient()->listEdgeIndexes(spaceId).via(runner()).then(
        [this, spaceId](StatusOr<std::vector<meta::cpp2::IndexItem>> resp) {
            if (!resp.ok()) {
                LOG(ERROR) << "SpaceId: " << spaceId << ", Show edge indexes failed"
                           << resp.status();
                return resp.status();
            }

            auto edgeIndexItems = std::move(resp).value();

            DataSet dataSet;
            dataSet.colNames = {"Names"};
            std::set<std::string> orderEdgeIndexNames;
            for (auto &edgeIndex : edgeIndexItems) {
                orderEdgeIndexNames.emplace(edgeIndex.get_index_name());
            }
            for (auto &name : orderEdgeIndexNames) {
                Row row;
                row.values.emplace_back(name);
                dataSet.rows.emplace_back(std::move(row));
            }
            return finish(ResultBuilder()
                              .value(Value(std::move(dataSet)))
                              .iter(Iterator::Kind::kDefault)
                              .finish());
        });
}

}   // namespace graph
}   // namespace nebula
