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

        nfa::MiniNfa<IteratorType> * lhs = u->lhs->accept(this);
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

        nfa::MiniNfa<IteratorType> * rhs = u->rhs->accept(this);
        assert(rhs);

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
        auto * ret = new nfa::MiniNfa<IteratorType>();

        nfa::MiniNfa<IteratorType> * lhs = c->lhs->accept(this);
        assert(lhs);

        nfa::MiniNfa<IteratorType> * rhs = c->rhs->accept(this);
        assert(rhs);

        // Epsilon transition from start state to lhs
        ret->start_state.out_edges.push_back({
                .callable = {},
                .to = &lhs->start_state,
        });

        // Epsilon transition from LHS to end state
        lhs->end_state.out_edges.push_back({
                .callable = {},
                .to = &rhs->start_state,
        });

        // Epsilon transition from RHS to end state
        rhs->end_state.out_edges.push_back({
                .callable = {},
                .to = &ret->end_state,
        });

        return ret;
}

template <typename IteratorType>
nfa::MiniNfa<IteratorType> *
GRVisitor<IteratorType>::visit(const KleeneStar<IteratorType> * k)
{
        auto * ret = new nfa::MiniNfa<IteratorType>();

        // Epsilon transition from start state to end_state
        ret->start_state.out_edges.push_back({
                .callable = {},
                .to = &ret->end_state,
        });

        nfa::MiniNfa<IteratorType> * operand = k->operand->accept(this);
        assert(operand);

        // Epsilon transition from start state to operand
        ret->start_state.out_edges.push_back({
                .callable = {},
                .to = &operand->start_state,
        });

        // Epsilon transition from operand end state to start_state
        operand->end_state.out_edges.push_back({
                .callable = {},
                .to = &ret->start_state,
        });

        // Epsilon transition from operand end state to start_state
        operand->end_state.out_edges.push_back({
                .callable = {},
                .to = &ret->end_state,
        });

        return ret;
}

// NOTE: all this shit is utterly fucked and will not type check. Unfuck the NFA type hierarchy

template <typename IteratorType>
void traverse_and_print(const nfa::State<IteratorType> * state, int indent = 0)
{
        for (int i = 0; i < indent; ++i)
                std::cout << "\t";

        std::cout << "State " << state << " has " << state->out_edges.size()
                  << " out edges";

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
void do_all_epsilon_transitions(std::set<nfa::State<IteratorType> *> & states)
{
        size_t states_count = states.size();
        while (true)
        {
                std::set<nfa::State<IteratorType> *> new_states;
                for (const auto state : states)
                {
                        for (auto [transition_predicate, eps_pos] :
                             state->out_edges)
                        {
                                if (!transition_predicate)
                                        new_states.insert(eps_pos);
                        }
                }

                for (const auto state : new_states)
                        states.insert(state);

                if (states.size() == states_count)
                        return;
                states_count = states.size();
        }
}

template <typename IteratorType>
bool apply_regex(
        const IteratorType begin,
        const IteratorType end,
        nfa::MiniNfa<IteratorType> * regex,
        bool print = false)
{
        using Position = nfa::State<IteratorType> *;

        std::set<Position> positions = {&regex->start_state};
        do_all_epsilon_transitions(positions);

        for (IteratorType it = begin; it != end; ++it)
        {
                if (print)
                {
                        std::cout << "Regex application iteration" << std::endl;
                        std::cout << "Currently at " << positions.size()
                                  << " positions" << std::endl;
                }
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
                do_all_epsilon_transitions(next_positions);

                positions = next_positions;
        }
        if (print)
        {
                std::cout << "Finished iteration" << std::endl;
                std::cout << "Currently at " << positions.size() << " positions"
                          << std::endl;
        }

        for (const Position end_pos : positions)
        {
                if (print)
                        std::cout << end_pos << std::endl;
                if (end_pos->accept)
                        return true;
        }

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

void test1_union(bool print = false)
{
        using It_T = std::vector<int>::const_iterator;

        trex::GRVisitor<It_T> v;
        trex::Predicate<It_T> p1([](It_T x) { return *x == 2; });
        trex::Predicate<It_T> p2([](It_T x) { return *x == 3; });
        trex::Union u(&p1, &p2);

        trex::nfa::MiniNfa<It_T> * res = u.accept(&v);
        res->end_state.accept = true;
        assert(res->start_state.out_edges[0]
                       .to->out_edges[0]
                       .to->out_edges[0]
                       .to
               == &res->end_state);
        assert(res->start_state.out_edges[1]
                       .to->out_edges[0]
                       .to->out_edges[0]
                       .to
               == &res->end_state);
        if (print)
        {
                std::cout << "Constructed NFA" << std::endl;

                trex::traverse_and_print<It_T>(&res->start_state);
        }

        std::vector<int> vec{2};
        std::vector<int> vec2{3};

        bool result = trex::apply_regex(vec.cbegin(), vec.cend(), res, print);
        if (print)
                printf("Regex 2 | 3 holds? %s\n", result ? "yes" : "no");
        assert(result);

        bool result2
                = trex::apply_regex(vec2.cbegin(), vec2.cend(), res, print);
        if (print)
                printf("Regex 2 | 3 holds? %s\n", result2 ? "yes" : "no");
        assert(result2);
}

void test2_concat(bool print = false)
{
        using It_T = std::vector<int>::const_iterator;

        trex::GRVisitor<It_T> v;
        trex::Predicate<It_T> p1([](It_T x) { return *x == 2; });
        trex::Predicate<It_T> p2([](It_T x) { return *x == 3; });
        trex::Concatenation c(&p1, &p2);

        trex::nfa::MiniNfa<It_T> * res = c.accept(&v);
        res->end_state.accept = true;
        if (print)
        {
                std::cout << "Constructed NFA" << std::endl;

                trex::traverse_and_print<It_T>(&res->start_state);
        }

        std::vector<int> vec{2, 3};

        bool result = trex::apply_regex(vec.cbegin(), vec.cend(), res, print);
        if (print)
                printf("Regex 23 holds? %s\n", result ? "yes" : "no");
        assert(result);
}

void test3_kleenestar(bool print = false)
{
        using It_T = std::vector<int>::const_iterator;

        trex::GRVisitor<It_T> v;
        trex::Predicate<It_T> p([](It_T x) { return *x == 2; });
        trex::KleeneStar k(&p);

        trex::nfa::MiniNfa<It_T> * res = k.accept(&v);
        res->end_state.accept = true;
        if (print)
        {
                std::cout << "Constructed NFA" << std::endl;

                trex::traverse_and_print<It_T>(&res->start_state);
        }

        std::vector<int> vec{};

        for (int i = 0; i < 10; ++i)
        {
                bool result = trex::apply_regex(vec.cbegin(), vec.cend(), res, print);
                if (print)
                        printf("Regex 2* holds? %s\n", result ? "yes" : "no");
                assert(result);

                vec.push_back(2);
        }
}

int main()
{
        test0_predicate();

        std::cout << "Finished test0 successfully" << std::endl;

        test1_union();

        std::cout << "Finished test1 successfully" << std::endl;

        test2_concat();

        std::cout << "Finished test2 successfully" << std::endl;

        test3_kleenestar();

        std::cout << "Finished test3 successfully" << std::endl;

        return 0;
}
