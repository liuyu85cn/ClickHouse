#pragma once

#include <Core/Types.h>

#include <memory>
#include <vector>
#include <Interpreters/ExpressionActions.h>
#include <Interpreters/AggregateDescription.h>
#include <Parsers/IAST_fwd.h>
#include <Storages/ColumnsDescription.h>

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>

namespace DB
{
struct StorageInMemoryMetadata;
using StorageMetadataPtr = std::shared_ptr<const StorageInMemoryMetadata>;

/// Description of projections for Storage
struct SecondaryIndexDescription
{
    enum class Type
    {
        Normal,
        Aggregate,
    };

    static constexpr const char * MINMAX_COUNT_PROJECTION_NAME = "_minmax_count_projection";

    /// Definition AST of projection
    ASTPtr definition_ast;

    /// Subquery AST for projection calculation
    ASTPtr query_ast;

    /// SecondaryIndex name
    String name;

    /// SecondaryIndex type (normal, aggregate, etc.)
    Type type = Type::Normal;

    /// Columns which are required for query_ast.
    Names required_columns;

    Names getRequiredColumns() const { return required_columns; }

    /// Sample block with projection columns. (NOTE: columns in block are empty, but not nullptr)
    Block sample_block;

    Block sample_block_for_keys;

    StorageMetadataPtr metadata;

    size_t key_size = 0;

    bool is_minmax_count_projection = false;

    /// If a primary key expression is used in the minmax_count projection, store the name of max expression.
    String primary_key_max_column_name;

    /// Stores partition value indices of partition value row. It's needed because identical
    /// partition columns will appear only once in projection block, but every column will have a
    /// value in the partition value row. This vector holds the biggest value index of give
    /// partition columns.
    std::vector<size_t> partition_value_indices;

    /// Parse projection from definition AST
    static SecondaryIndexDescription
    getSecondaryIndexFromAST(const ASTPtr & definition_ast, const ColumnsDescription & columns, ContextPtr query_context);

    static SecondaryIndexDescription getMinMaxCountSecondaryIndex(
        const ColumnsDescription & columns,
        ASTPtr partition_columns,
        const Names & minmax_columns,
        const ASTs & primary_key_asts,
        ContextPtr query_context);

    SecondaryIndexDescription() = default;

    /// We need custom copy constructors because we don't want
    /// unintentionally share AST variables and modify them.
    SecondaryIndexDescription(const SecondaryIndexDescription & other) = delete;
    SecondaryIndexDescription(SecondaryIndexDescription && other) = default;
    SecondaryIndexDescription & operator=(const SecondaryIndexDescription & other) = delete;
    SecondaryIndexDescription & operator=(SecondaryIndexDescription && other) = default;

    SecondaryIndexDescription clone() const;

    bool operator==(const SecondaryIndexDescription & other) const;
    bool operator!=(const SecondaryIndexDescription & other) const { return !(*this == other); }

    /// Recalculate projection with new columns because projection expression may change
    /// if something change in columns.
    void recalculateWithNewColumns(const ColumnsDescription & new_columns, ContextPtr query_context);

    bool isPrimaryKeyColumnPossiblyWrappedInFunctions(const ASTPtr & node) const;

    Block calculate(const Block & block, ContextPtr context) const;

    String getDirectoryName() const { return name + ".proj"; }
};

using SecondaryIndexDescriptionRawPtr = const SecondaryIndexDescription *;

/// All projections in storage
struct SecondaryIndexsDescription : public IHints<1, SecondaryIndexsDescription>
{
    SecondaryIndexsDescription() = default;
    SecondaryIndexsDescription(SecondaryIndexsDescription && other) = default;
    SecondaryIndexsDescription & operator=(SecondaryIndexsDescription && other) = default;

    SecondaryIndexsDescription clone() const;

    /// Convert description to string
    String toString() const;
    /// Parse description from string
    static SecondaryIndexsDescription parse(const String & str, const ColumnsDescription & columns, ContextPtr query_context);

    /// Return common expression for all stored projections
    ExpressionActionsPtr getSingleExpressionForSecondaryIndexs(const ColumnsDescription & columns, ContextPtr query_context) const;

    bool operator==(const SecondaryIndexsDescription & other) const { return projections == other.projections; }
    bool operator!=(const SecondaryIndexsDescription & other) const { return !(*this == other); }

    auto begin() const { return projections.begin(); }
    auto end() const { return projections.end(); }

    size_t size() const { return projections.size(); }
    bool empty() const { return projections.empty(); }

    bool has(const String & projection_name) const;
    const SecondaryIndexDescription & get(const String & projection_name) const;

    void
    add(SecondaryIndexDescription && projection, const String & after_projection = String(), bool first = false, bool if_not_exists = false);
    void remove(const String & projection_name, bool if_exists);

    std::vector<String> getAllRegisteredNames() const override;

private:
    /// Keep the sequence of columns and allow to lookup by name.
    using Container = std::list<SecondaryIndexDescription>;
    using Map = std::unordered_map<std::string, Container::iterator>;

    Container projections;
    Map map;
};

}
