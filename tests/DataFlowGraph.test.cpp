// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "Luau/DataFlowGraph.h"
#include "Luau/Error.h"
#include "Luau/Parser.h"

#include "AstQueryDsl.h"
#include "ScopedFlags.h"

#include "doctest.h"

using namespace Luau;

struct DataFlowGraphFixture
{
    // Only needed to fix the operator== reflexivity of an empty Symbol.
    ScopedFastFlag dcr{"DebugLuauDeferredConstraintResolution", true};

    InternalErrorReporter handle;

    Allocator allocator;
    AstNameTable names{allocator};
    AstStatBlock* module;

    std::optional<DataFlowGraph> graph;

    void dfg(const std::string& code)
    {
        ParseResult parseResult = Parser::parse(code.c_str(), code.size(), names, allocator);
        if (!parseResult.errors.empty())
            throw ParseErrors(std::move(parseResult.errors));
        module = parseResult.root;
        graph = DataFlowGraphBuilder::build(module, NotNull{&handle});
    }

    template<typename T, int N>
    DefId getDef(const std::vector<Nth>& nths = {nth<T>(N)})
    {
        T* node = query<T, N>(module, nths);
        REQUIRE(node);
        return graph->getDef(node);
    }
};

TEST_SUITE_BEGIN("DataFlowGraphBuilder");

TEST_CASE_FIXTURE(DataFlowGraphFixture, "define_locals_in_local_stat")
{
    dfg(R"(
        local x = 5
        local y = x
    )");

    (void)getDef<AstExprLocal, 1>();
}

TEST_CASE_FIXTURE(DataFlowGraphFixture, "define_parameters_in_functions")
{
    dfg(R"(
        local function f(x)
            local y = x
        end
    )");

    (void)getDef<AstExprLocal, 1>();
}

TEST_CASE_FIXTURE(DataFlowGraphFixture, "find_aliases")
{
    dfg(R"(
        local x = 5
        local y = x
        local z = y
    )");

    DefId x = getDef<AstExprLocal, 1>();
    DefId y = getDef<AstExprLocal, 2>();
    REQUIRE(x != y);
}

TEST_CASE_FIXTURE(DataFlowGraphFixture, "independent_locals")
{
    dfg(R"(
        local x = 5
        local y = 5

        local a = x
        local b = y
    )");

    DefId x = getDef<AstExprLocal, 1>();
    DefId y = getDef<AstExprLocal, 2>();
    REQUIRE(x != y);
}

TEST_SUITE_END();
