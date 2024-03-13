
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
        // TODO: determine callable type

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

struct GRVisitor
{
        void visit(Predicate * p)
        {
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

