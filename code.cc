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
        std::function callable;

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

namespace nfa
{

struct Transition
{
        // TODO: how to represent epsilon transition?
        std::function predicate;
        // TODO: rename
        State pointing_to;
};

using State = std::vector<Transition>;

}; // namespace nfa

struct GRVisitor
{
        std::stack<State> constructed {};

        // NOTE: nicety: we know that predicates are going to be the leaf nodes of the syntax tree
        void visit(Predicate * p)
        {
                constructed.push({{ .predicate =  callable, .pointing_to = {} });
        }

        void visit(Union * u)
        {
                
        }

        void visit(Concatenation * c)
        {
        }

        void visit(KleeneStar * k)
        {
        }
};

