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

