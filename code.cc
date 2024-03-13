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

struct MiniNfa;

struct Transition
{
        std::function<bool> callable;

        std::shared_ptr<nfa::MiniNfa> pointed_to;
};

using State = std::vector<Transition>;

struct MiniNfa
{
        State start_state {};
        State end_state {};
};


}; // namespace nfa

struct GRVisitor
{
        // NOTE: nicety: we know that predicates are going to be the leaf nodes of the syntax tree
        std::shared_ptr<nfa::MiniNfa> visit(Predicate * p)
        {
                nfa::MiniNfa ret;
                ret.start_state.push_back({ .callable = p->callable, .pointed_to = std::make_shared(ret.end_state) });

                return std::make_shared(ret);
        }

        std::shared_ptr<nfa::MiniNfa> visit(Union * u)
        {
                // TODO: determine if this can be a stack machine
                nfa::MiniNfa ret;
                std::shared_ptr<nfa::MiniNfa> end = std::make_shared(ret.end_state);

                ret.start_state.push_back({ .callable = impl::EPSILON, .pointed_to = u->lhs->accept(this), });
                ret.start_state.back().pointed_to->end_state = end;
                ret.start_state.push_back({ .callable = impl::EPSILON, .pointed_to = u->rhs->accept(this), });
                ret.start_state.back().pointed_to->end_state = end;

                return ret;
        }

        std::shared_ptr<nfa::MiniNfa> visit(Concatenation * c)
        {
                nfa::MiniNfa ret;
                std::shared_ptr<nfa::MiniNfa> end = std::make_shared(ret.end_state);

                ret.start_state.push_back({ .callable = impl::EPSILON, .pointed_to = u->lhs->accept(this), });
                ret.start_state.back().pointed_to->end_state.push_back({ .callable = impl::EPSILON,
                        .pointed_to = u->rhs->accept(this), });
                ret.start_state.back().pointed_to->end_state.back().pointed_to->end_state = end;

                return ret;
        }

        std::shared_ptr<nfa::MiniNfa> visit(KleeneStar * k)
        {
                nfa::MiniNfa ret;
                std::shared_ptr<nfa::MiniNfa> start = std::make_shared(ret.start_state);
                std::shared_ptr<nfa::MiniNfa> end = std::make_shared(ret.end_state);

                ret.start_state.push_back({ .callable = impl::EPSILON, .pointed_to = u->operand->accept(this), });
                ret.start_state.back().pointed_to->end_state = end;
                

                return ret;
        }
};

// NOTE: all this shit is utterly fucked and will not type check. Unfuck the NFA type hierarchy

void traverse_and_print(std::shared_ptr<nfa::MiniNfa> expr, int indent = 0)
{
        for (int i = 0; i < indent; ++i)
                std::cout << "\t";

        std::cout << "Start state has " << expr.start_state.size() << " out edges" << std::endl;
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
        Predicate p1 { .callable = [](int x){ return x == 2; }, };
        Predicate p2 { .callable = [](int x){ return x == 3; }, };
        Union u { .lhs = &p1, .rhs = &p2, };
        std::shared_ptr<nfa::MiniNfa> res = u.accept(&v);

        traverse_and_print(res);
        return 0;
}

