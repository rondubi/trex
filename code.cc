#include <iostream>
#include <memory>
#include <stack>

struct Predicate;
struct Union;
struct Concatenation;
struct KleeneStar;

namespace nfa
{
struct MiniNfa;
};

struct GeneralizedRegex
{
        virtual std::shared_ptr<nfa::MiniNfa> accept(GRVisitor * v) const = 0;
};

struct Predicate : GeneralizedRegex
{
        // TODO: will this actually work?
        std::function<bool> callable;

        std::shared_ptr<nfa::MiniNfa> accept(GRVisitor * v) const;
        {
                return v->accept(this);
        }
};

struct Union : GeneralizedRegex
{
        GeneralizedRegex * lhs;
        GeneralizedRegex * rhs;

        std::shared_ptr<nfa::MiniNfa> accept(GRVisitor * v) const;
        {
                return v->accept(this);
        }
};

struct Concatenation : GeneralizedRegex
{
        GeneralizedRegex * lhs;
        GeneralizedRegex * rhs;

        std::shared_ptr<nfa::MiniNfa> accept(GRVisitor * v) const;
        {
                return v->accept(this);
        }
};

struct KleeneStar : GeneralizedRegex
{
        GeneralizedRegex * operand;

        std::shared_ptr<nfa::MiniNfa> accept(GRVisitor * v) const;
        {
                return v->accept(this);
        }
};

namespace impl
{

template <typename... Args>
bool EPSILON(Args... args)
{
        return true;
}

}; // namespace impl

namespace nfa
{

struct Transition;

struct State
{
        bool accept = false;
        std::vector<Transition> out_edges;
};

struct Transition
{
        std::function<bool> callable;
        std::shared_ptr<State> to;
};

struct MiniNfa
{
        State start_state{};
        State end_state{};
};

}; // namespace nfa

struct GRVisitor
{
        // NOTE: nicety: we know that predicates are going to be the leaf nodes of the syntax tree
        std::shared_ptr<nfa::MiniNfa> visit(Predicate * p)
        {
                nfa::MiniNfa ret;

                ret.start_state.out_edges.push_back({
                        .callable = p->callable,
                        .to = std::make_shared<nfa::State>(ret.end_state),
                });

                return std::make_shared<nfa::MiniNfa>(ret);
        }

        std::shared_ptr<nfa::MiniNfa> visit(Union * u)
        {
                nfa::MiniNfa ret;
                std::shared_ptr<nfa::State> end_ptr = std::make_shared<nfa::State>(ret.end_state);

                std::shared_ptr<nfa::MiniNfa> lhs = u->lhs->accept(this);
                lhs->end_state.out_edges.push_back({ .callable = impl::EPSILON, .to = end_ptr, });
                std::shared_ptr<nfa::MiniNfa> rhs = u->rhs->accept(this);
                rhs->end_state.out_edges.push_back({ .callable = impl::EPSILON, .to = end_ptr, });

                return std::make_shared<nfa::MiniNfa>(ret);
        }

        std::shared_ptr<nfa::MiniNfa> visit(Concatenation * c)
        {
                return NULL;
        }

        std::shared_ptr<nfa::MiniNfa> visit(KleeneStar * k)
        {
                return NULL;
        }
};

// NOTE: all this shit is utterly fucked and will not type check. Unfuck the NFA type hierarchy

void traverse_and_print(std::shared_ptr<nfa::MiniNfa> expr, int indent = 0)
{
        for (int i = 0; i < indent; ++i)
                std::cout << "\t";

        std::cout << "Start state has " << expr.start_state.size()
                  << " out edges" << std::endl;
        for (const auto [p, stage] : expr.start_state)
        {
                traverse_and_print(stage, indent + 1);
                std::cout << std::endl;
        }
        std::cout << "End subexpr" << std::endl;
}

int main()
{
        GRVisitor v;
        Predicate p1{
                .callable = [](int x) { return x == 2; },
        };
        Predicate p2{
                .callable = [](int x) { return x == 3; },
        };
        Union u{
                .lhs = &p1,
                .rhs = &p2,
        };
        std::shared_ptr<nfa::MiniNfa> res = u.accept(&v);

        traverse_and_print(res);
        return 0;
}
