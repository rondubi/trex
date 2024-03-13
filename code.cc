#include <stack>

struct Predicate;
struct Union;
struct Concatenation;
struct KleeneStar;

struct GeneralizedRegex
{
        virtual void accept(GRVisitor * v) const = 0;
};

struct Predicate : GeneralizedRegex
{
        // TODO: will this actually work?
        std::function<bool> callable;

        void accept(GRVisitor * v) const;
        {
                v->accept(this);
        }
};

struct Union : GeneralizedRegex
{
        GeneralizedRegex * lhs;
        GeneralizedRegex * rhs;

        void accept(GRVisitor * v) const;
        {
                v->accept(this);
        }
};

struct Concatenation : GeneralizedRegex
{
        GeneralizedRegex * lhs;
        GeneralizedRegex * rhs;

        void accept(GRVisitor * v) const;
        {
                v->accept(this);
        }
};

struct KleeneStar : GeneralizedRegex
{
        GeneralizedRegex * operand;

        void accept(GRVisitor * v) const;
        {
                v->accept(this);
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

        std::shared_ptr<MiniNfa> pointed_to;
};

using State = std::vector<Transition>;

struct MiniNfa
{
        State start_state {};
        State end_state {};
};

std::shared_ptr<MiniNfa> make_epsilon()
{
        MiniNfa ret;
        ret.start_state.push_back({ .callable = impl::EPSILON, .pointed_to = std::make_shared(ret.end_state) });
        
        return std::make_shared(ret);
}

// TODO: rename
std::shared_ptr<MiniNfa> make_pred_transition(std::function callable)
{
        MiniNfa ret;
        ret.start_state.push_back({ .callable = callable, .pointed_to = std::make_shared(ret.end_state) });

        return std::make_shared(ret);
}

}; // namespace nfa

struct GRVisitor
{
        std::stack<> constructed {};

        // NOTE: nicety: we know that predicates are going to be the leaf nodes of the syntax tree
        void visit(Predicate * p)
        {
        }

        void visit(Union * u)
        {
                // TODO: determine if this can be a stack machine
                u->lhs->accept(this);
                u->rhs->accept(this);

        }

        void visit(Concatenation * c)
        {
                u->lhs->accept(this);
                u->rhs->accept(this);

        }

        void visit(KleeneStar * k)
        {
                u->operand->accept(this);
        }
};

