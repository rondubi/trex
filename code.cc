#include <any>
#include <cassert>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <stack>

namespace trex
{

namespace nfa
{
template <typename IteratorType>
struct MiniNfa;
};

template <typename IteratorType>
struct Predicate;

template <typename IteratorType>
struct Union;

template <typename IteratorType>
struct Concatenation;

template <typename IteratorType>
struct KleeneStar;

template <typename IteratorType>
struct GRVisitor
{
        nfa::MiniNfa<IteratorType> * visit(const Predicate<IteratorType> *);
        nfa::MiniNfa<IteratorType> * visit(const Union<IteratorType> *);
        nfa::MiniNfa<IteratorType> * visit(const Concatenation<IteratorType> *);
        nfa::MiniNfa<IteratorType> * visit(const KleeneStar<IteratorType> *);
};

template <typename IteratorType>
struct GeneralizedRegex
{
        virtual nfa::MiniNfa<IteratorType> *
        accept(GRVisitor<IteratorType> * v) const
                = 0;
};

template <typename IteratorType>
struct Predicate : GeneralizedRegex<IteratorType>
{
        std::function<bool(IteratorType)> callable;

        Predicate(std::function<bool(IteratorType)> c_) : callable(c_) { }

        nfa::MiniNfa<IteratorType> * accept(GRVisitor<IteratorType> * v) const
        {
                return v->visit(this);
        }
};

template <typename IteratorType>
struct Union : GeneralizedRegex<IteratorType>
{
        GeneralizedRegex<IteratorType> * lhs;
        GeneralizedRegex<IteratorType> * rhs;

        Union(GeneralizedRegex<IteratorType> * l_,
              GeneralizedRegex<IteratorType> * r_)
            : lhs(l_), rhs(r_)
        {
        }

        nfa::MiniNfa<IteratorType> * accept(GRVisitor<IteratorType> * v) const
        {
                return v->visit(this);
        }
};

template <typename IteratorType>
struct Concatenation : GeneralizedRegex<IteratorType>
{
        GeneralizedRegex<IteratorType> * lhs;
        GeneralizedRegex<IteratorType> * rhs;

        Concatenation(
                GeneralizedRegex<IteratorType> * l_,
                GeneralizedRegex<IteratorType> * r_)
            : lhs(l_), rhs(r_)
        {
        }

        nfa::MiniNfa<IteratorType> * accept(GRVisitor<IteratorType> * v) const
        {
                return v->visit(this);
        }
};

template <typename IteratorType>
struct KleeneStar : GeneralizedRegex<IteratorType>
{
        GeneralizedRegex<IteratorType> * operand;

        KleeneStar(GeneralizedRegex<IteratorType> * o_) : operand(o_) { }

        nfa::MiniNfa<IteratorType> * accept(GRVisitor<IteratorType> * v) const
        {
                return v->visit(this);
        }
};

namespace nfa
{

template <typename IteratorType>
struct Transition;

template <typename IteratorType>
struct State
{
        bool accept = false;
        std::vector<Transition<IteratorType>> out_edges{};
};

template <typename IteratorType>
struct Transition
{
        std::optional<std::function<bool(IteratorType)>> callable;
        State<IteratorType> * to;
};

template <typename IteratorType>
struct MiniNfa
{
        State<IteratorType> start_state{};
        State<IteratorType> end_state{};
};

}; // namespace nfa

// NOTE: nicety: we know that predicates are going to be the leaf nodes of the syntax tree
template <typename IteratorType>
nfa::MiniNfa<IteratorType> *
GRVisitor<IteratorType>::visit(const Predicate<IteratorType> * p)
{
        auto * ret = new nfa::MiniNfa<IteratorType>();

        ret->start_state.out_edges.push_back({
                .callable = p->callable,
                .to = &ret->end_state,
        });

        assert(ret->start_state.out_edges.size() == 1);
        assert(ret->start_state.out_edges[0].to == &ret->end_state);

        return ret;
}

template <typename IteratorType>
nfa::MiniNfa<IteratorType> *
GRVisitor<IteratorType>::visit(const Union<IteratorType> * u)
{
        auto * ret = new nfa::MiniNfa<IteratorType>();

        auto * lhs = u->lhs->accept(this);
        assert(lhs);

        // Epsilon transition from LHS to end state
        lhs->end_state.out_edges.push_back({
                .callable = {},
                .to = &ret->end_state,
        });

        // Epsilon transition from start state to lhs
        ret->start_state.out_edges.push_back({
                .callable = {},
                .to = &lhs->start_state,
        });

        auto * rhs = u->rhs->accept(this);

        // Epsilon transition from RHS to end state
        rhs->end_state.out_edges.push_back({
                .callable = {},
                .to = &ret->end_state,
        });

        // Epsilon transition from end state to rhs
        ret->start_state.out_edges.push_back({
                .callable = {},
                .to = &rhs->start_state,
        });

        assert(ret->start_state.out_edges.size() == 2);

        return ret;
}

template <typename IteratorType>
nfa::MiniNfa<IteratorType> *
GRVisitor<IteratorType>::visit(const Concatenation<IteratorType> * c)
{
        return NULL;
}

template <typename IteratorType>
nfa::MiniNfa<IteratorType> *
GRVisitor<IteratorType>::visit(const KleeneStar<IteratorType> * k)
{
        return NULL;
}

// NOTE: all this shit is utterly fucked and will not type check. Unfuck the NFA type hierarchy

template <typename IteratorType>
void traverse_and_print(const nfa::State<IteratorType> * state, int indent = 0)
{
        for (int i = 0; i < indent; ++i)
                std::cout << "\t";

        std::cout << "State has " << state->out_edges.size() << " out edges";

        int count_epsilon = 0;
        for (const auto [p, stage] : state->out_edges)
                if (!p)
                        ++count_epsilon;
        std::cout << ", " << count_epsilon
                  << " of these are epsilon transitions.";

        if (state->accept)
                std::cout << " This is an accept state.";

        std::cout << std::endl;

        for (const auto [p, stage] : state->out_edges)
                traverse_and_print<IteratorType>(stage, indent + 1);

        for (int i = 0; i < indent; ++i)
                std::cout << "\t";

        std::cout << "End subexpr" << std::endl;
}

template <typename IteratorType>
bool apply_regex(
        const IteratorType begin,
        const IteratorType end,
        nfa::MiniNfa<IteratorType> * regex)
{
        using Position = nfa::State<IteratorType> *;

        std::set<Position> positions = {&regex->start_state};
        for (auto [transition_predicate, eps_pos] :
             regex->start_state.out_edges)
        {
                if (!transition_predicate)
                        positions.insert(eps_pos);
        }

        for (IteratorType it = begin; it != end; ++it)
        {
                // std::cout << "Regex application iteration" << std::endl;
                // std::cout << "Currently at " << positions.size() << " positions"
                // << std::endl;
                std::set<Position> next_positions;
                for (const Position pos : positions)
                {
                        for (const auto [transition_predicate, next_pos] :
                             pos->out_edges)
                        {
                                if (!transition_predicate
                                    || transition_predicate.value()(it))
                                        next_positions.insert(next_pos);
                        }
                }

                positions = next_positions;
        }
        // std::cout << "Finished iteration" << std::endl;
        // std::cout << "Currently at " << positions.size() << " positions"
        // << std::endl;

        for (const Position end_pos : positions)
                if (end_pos->accept)
                        return true;

        return false;
}

}; // namespace trex

void test0_predicate(bool print = false)
{
        using It_T = std::vector<int>::const_iterator;

        trex::GRVisitor<It_T> v;
        trex::Predicate<It_T> p([](It_T x) { return *x == 2; });

        trex::nfa::MiniNfa<It_T> * res = p.accept(&v);
        assert(res->start_state.out_edges[0].to == &res->end_state);
        res->end_state.accept = true;

        if (print)
                trex::traverse_and_print<It_T>(&res->start_state);

        std::vector<int> vec{2};
        std::vector<int> vec2{3};

        bool result = trex::apply_regex(vec.cbegin(), vec.cend(), res);
        if (print)
                printf("Regex 2 holds? %s\n", result ? "yes" : "no");
        assert(result);

        bool result2 = trex::apply_regex(vec2.cbegin(), vec2.cend(), res);
        if (print)
                printf("Regex 3 holds? %s\n", result2 ? "yes" : "no");
        assert(!result2);
}

void test1_union()
{
        using It_T = std::vector<int>::const_iterator;

        trex::GRVisitor<It_T> v;
        trex::Predicate<It_T> p1([](It_T x) { return *x == 2; });
        trex::Predicate<It_T> p2([](It_T x) { return *x == 3; });
        trex::Union u(&p1, &p2);

        trex::nfa::MiniNfa<It_T> * res = u.accept(&v);
        res->end_state.accept = true;
        std::cout << "Constructed NFA" << std::endl;

        trex::traverse_and_print<It_T>(&res->start_state);

        std::vector<int> vec{2};

        bool result = trex::apply_regex(vec.cbegin(), vec.cend(), res);
        printf("Regex 2 | 3 holds? %s\n", result ? "yes" : "no");
        assert(result);
}

int main()
{
        test0_predicate();

        test1_union();

        return 0;
}
