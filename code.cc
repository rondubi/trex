#include <any>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
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
        std::shared_ptr<nfa::MiniNfa<IteratorType>>
        visit(const Predicate<IteratorType> *);
        std::shared_ptr<nfa::MiniNfa<IteratorType>>
        visit(const Union<IteratorType> *);
        std::shared_ptr<nfa::MiniNfa<IteratorType>>
        visit(const Concatenation<IteratorType> *);
        std::shared_ptr<nfa::MiniNfa<IteratorType>>
        visit(const KleeneStar<IteratorType> *);
};

template <typename IteratorType>
struct GeneralizedRegex
{
        virtual std::shared_ptr<nfa::MiniNfa<IteratorType>>
        accept(GRVisitor<IteratorType> * v) const = 0;
};

template <typename IteratorType>
struct Predicate : GeneralizedRegex<IteratorType>
{
        std::function<bool(IteratorType)> callable;

        Predicate(std::function<bool(IteratorType)> c_) : callable(c_) { }

        std::shared_ptr<nfa::MiniNfa<IteratorType>>
        accept(GRVisitor<IteratorType> * v) const
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

        std::shared_ptr<nfa::MiniNfa<IteratorType>>
        accept(GRVisitor<IteratorType> * v) const
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

        std::shared_ptr<nfa::MiniNfa<IteratorType>>
        accept(GRVisitor<IteratorType> * v) const
        {
                return v->visit(this);
        }
};

template <typename IteratorType>
struct KleeneStar : GeneralizedRegex<IteratorType>
{
        GeneralizedRegex<IteratorType> * operand;

        KleeneStar(GeneralizedRegex<IteratorType> * o_) : operand(o_) { }

        std::shared_ptr<nfa::MiniNfa<IteratorType>>
        accept(GRVisitor<IteratorType> * v) const
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
        std::shared_ptr<State<IteratorType>> to;
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
std::shared_ptr<nfa::MiniNfa<IteratorType>>
GRVisitor<IteratorType>::visit(const Predicate<IteratorType> * p)
{
        std::shared_ptr<nfa::MiniNfa<IteratorType>> ret = std::make_shared<nfa::MiniNfa<IteratorType>>();

        ret->start_state.out_edges.push_back({
                .callable = p->callable,
                .to = std::shared_ptr<nfa::State<IteratorType>>(&ret->end_state, [](auto x){return;}),
        });

        assert(ret->start_state.out_edges.size() == 1);
        assert(ret->start_state.out_edges[0].to.get() == &ret->end_state);

        return ret;
}

template <typename IteratorType>
std::shared_ptr<nfa::MiniNfa<IteratorType>>
GRVisitor<IteratorType>::visit(const Union<IteratorType> * u)
{
        nfa::MiniNfa<IteratorType> ret;
        std::shared_ptr<nfa::State<IteratorType>> end_ptr
                = std::make_shared<nfa::State<IteratorType>>(ret.end_state);

        std::shared_ptr<nfa::MiniNfa<IteratorType>> lhs = u->lhs->accept(this);
        assert(lhs);

        // Epsilon transition from LHS to end state
        lhs->end_state.out_edges.push_back({
                .callable = {},
                .to = end_ptr,
        });

        // Epsilon transition from start state to lhs
        ret.start_state.out_edges.push_back({
                .callable = {},
                .to
                = std::make_shared<nfa::State<IteratorType>>(lhs->start_state),
        });

        std::shared_ptr<nfa::MiniNfa<IteratorType>> rhs = u->rhs->accept(this);
        assert(rhs);

        // Epsilon transition from RHS to end state
        rhs->end_state.out_edges.push_back({
                .callable = {},
                .to = end_ptr,
        });

        // Epsilon transition from end state to rhs
        ret.start_state.out_edges.push_back({
                .callable = {},
                .to
                = std::make_shared<nfa::State<IteratorType>>(rhs->start_state),
        });

        assert(ret.start_state.out_edges.size() == 2);

        return std::make_shared<nfa::MiniNfa<IteratorType>>(ret);
}

template <typename IteratorType>
std::shared_ptr<nfa::MiniNfa<IteratorType>>
GRVisitor<IteratorType>::visit(const Concatenation<IteratorType> * c)
{
        return NULL;
}

template <typename IteratorType>
std::shared_ptr<nfa::MiniNfa<IteratorType>>
GRVisitor<IteratorType>::visit(const KleeneStar<IteratorType> * k)
{
        return NULL;
}

// NOTE: all this shit is utterly fucked and will not type check. Unfuck the NFA type hierarchy

template <typename IteratorType>
void traverse_and_print(
        const std::shared_ptr<nfa::State<IteratorType>> state, int indent = 0)
{
        for (int i = 0; i < indent; ++i)
                std::cout << "\t";

        std::cout << "State has " << state->out_edges.size()
                  << " out edges";

        int count_epsilon = 0;
        for (const auto [p, stage] : state->out_edges)
                if (!p)
                        ++count_epsilon;
        std::cout << ", " << count_epsilon << " of these are epsilon transitions.";

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
        std::shared_ptr<nfa::MiniNfa<IteratorType>> regex)
{
        using Position = std::shared_ptr<nfa::State<IteratorType>>;

        std::set<Position> positions = {
                std::make_shared<nfa::State<IteratorType>>(regex->start_state)};
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

        std::shared_ptr<trex::nfa::MiniNfa<It_T>> res = p.accept(&v);
        assert(res->start_state.out_edges[0].to.get() == &res->end_state);
        res->end_state.accept = true;

        if (print)
                trex::traverse_and_print<It_T>(
                        std::make_shared<trex::nfa::State<It_T>>(res->start_state));

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

void test1()
{
        using It_T = std::vector<int>::const_iterator;

        trex::GRVisitor<It_T> v;
        trex::Predicate<It_T> p1([](It_T x) { return *x == 2; });
        trex::Predicate<It_T> p2([](It_T x) { return *x == 3; });
        trex::Union u(&p1, &p2);

        std::shared_ptr<trex::nfa::MiniNfa<It_T>> res = u.accept(&v);
        res->end_state.accept = true;
        std::cout << "Constructed NFA" << std::endl;

        trex::traverse_and_print<It_T>(
                std::make_shared<trex::nfa::State<It_T>>(res->start_state));

        std::vector<int> vec{2};

        bool result = trex::apply_regex(vec.cbegin(), vec.cend(), res);
        printf("Regex 2 | 3 holds? %s\n", result ? "yes" : "no");
        assert(result);
}

int main()
{
        test0_predicate();

        test1();

        return 0;
}

