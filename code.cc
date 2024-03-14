#include <any>
#include <functional>
#include <iostream>
#include <memory>
#include <stack>

namespace nfa
{
template <typename IteratorType>
struct MiniNfa;
};

template <typename IteratorType>
struct Predicate;

struct Union;
struct Concatenation;
struct KleeneStar;

template <typename IteratorType>
struct GRVisitor
{
        std::shared_ptr<nfa::MiniNfa<IteratorType>> visit(const Predicate<IteratorType> *);
        std::shared_ptr<nfa::MiniNfa<IteratorType>> visit(const Union *);
        std::shared_ptr<nfa::MiniNfa<IteratorType>> visit(const Concatenation *);
        std::shared_ptr<nfa::MiniNfa<IteratorType>> visit(const KleeneStar *);
};

struct GeneralizedRegex
{
        template <typename IteratorType>
        std::shared_ptr<nfa::MiniNfa<IteratorType>> accept(GRVisitor<IteratorType> * v) const
        {
                return NULL;
        }
};

template <typename IteratorType>
struct Predicate : GeneralizedRegex
{
        // TODO: will this actually work?
        std::function<bool(IteratorType)> callable;

        Predicate(std::function<bool(IteratorType)> c_) : callable(c_) { }

        std::shared_ptr<nfa::MiniNfa<IteratorType>> accept(GRVisitor<IteratorType> * v) const
        {
                return v->visit(this);
        }
};

struct Union : GeneralizedRegex
{
        GeneralizedRegex * lhs;
        GeneralizedRegex * rhs;

        Union(GeneralizedRegex * l_, GeneralizedRegex * r_) : lhs(l_), rhs(r_)
        {
        }

        template <typename IteratorType>
        std::shared_ptr<nfa::MiniNfa<IteratorType>> accept(GRVisitor<IteratorType> * v) const
        {
                return v->visit(this);
        }
};

struct Concatenation : GeneralizedRegex
{
        GeneralizedRegex * lhs;
        GeneralizedRegex * rhs;

        Concatenation(GeneralizedRegex * l_, GeneralizedRegex * r_)
            : lhs(l_), rhs(r_)
        {
        }

        template <typename IteratorType>
        std::shared_ptr<nfa::MiniNfa<IteratorType>> accept(GRVisitor<IteratorType> * v) const
        {
                return v->visit(this);
        }
};

struct KleeneStar : GeneralizedRegex
{
        GeneralizedRegex * operand;

        KleeneStar(GeneralizedRegex * o_) : operand(o_) { }

        template <typename IteratorType>
        std::shared_ptr<nfa::MiniNfa<IteratorType>> accept(GRVisitor<IteratorType> * v) const
        {
                return v->visit(this);
        }
};

namespace impl
{

template <typename IteratorType>
bool EPSILON(IteratorType unused_arg)
{
        return true;
}

}; // namespace impl

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
        std::function<bool(IteratorType)> callable;
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
std::shared_ptr<nfa::MiniNfa<IteratorType>> GRVisitor<IteratorType>::visit(const Predicate<IteratorType> * p)
{
        nfa::MiniNfa<IteratorType> ret;

        ret.start_state.out_edges.push_back({
                .callable = p->callable,
                .to = std::make_shared<nfa::State<IteratorType>>(ret.end_state),
        });

        return std::make_shared<nfa::MiniNfa<IteratorType>>(ret);
}

template <typename IteratorType>
std::shared_ptr<nfa::MiniNfa<IteratorType>> GRVisitor<IteratorType>::visit(const Union * u)
{
        nfa::MiniNfa<IteratorType> ret;
        std::shared_ptr<nfa::State<IteratorType>> end_ptr
                = std::make_shared<nfa::State<IteratorType>>(ret.end_state);

        std::shared_ptr<nfa::MiniNfa<IteratorType>> lhs = u->lhs->accept(this);
        lhs->end_state.out_edges.push_back({
                .callable = impl::EPSILON<IteratorType>,
                .to = end_ptr,
        });
        std::shared_ptr<nfa::MiniNfa<IteratorType>> rhs = u->rhs->accept(this);
        rhs->end_state.out_edges.push_back({
                .callable = impl::EPSILON<IteratorType>,
                .to = end_ptr,
        });

        return std::make_shared<nfa::MiniNfa<IteratorType>>(ret);
}

template <typename IteratorType>
std::shared_ptr<nfa::MiniNfa<IteratorType>> GRVisitor<IteratorType>::visit(const Concatenation * c) { return NULL; }

template <typename IteratorType>
std::shared_ptr<nfa::MiniNfa<IteratorType>> GRVisitor<IteratorType>::visit(const KleeneStar * k) { return NULL; }

// NOTE: all this shit is utterly fucked and will not type check. Unfuck the NFA type hierarchy

template <typename IteratorType>
void traverse_and_print(const std::shared_ptr<nfa::State<IteratorType>> state, int indent = 0)
{
        for (int i = 0; i < indent; ++i)
                std::cout << "\t";

        std::cout << "Start state has " << state->out_edges.size()
                  << " out edges" << std::endl;
        for (const auto [p, stage] : state->out_edges)
        {
                traverse_and_print<IteratorType>(stage, indent + 1);
                std::cout << std::endl;
        }
        std::cout << "End subexpr" << std::endl;
}

int main()
{
        using It_T = std::vector<int>::const_iterator;

        GRVisitor<It_T> v;
        Predicate<It_T> p1([](It_T x) { return *x == 2; });
        Predicate<It_T> p2([](It_T x) { return *x == 3; });
        Union u(&p1, &p2);
        std::shared_ptr<nfa::MiniNfa<It_T>> res = u.accept(&v);

        traverse_and_print<It_T>(std::make_shared<nfa::State<It_T>>(res->start_state));
        return 0;
}

