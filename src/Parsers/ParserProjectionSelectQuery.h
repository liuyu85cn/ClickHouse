#pragma once

#include <Parsers/IParserBase.h>


namespace DB
{


class ParserProjectionSelectQuery : public IParserBase
{
public:
    bool isSecondaryProjection = false;
    void setSecondaryProjection(bool flag);
protected:
    const char * getName() const override { return "PROJECTION SELECT query"; }
    bool parseImpl(Pos & pos, ASTPtr & node, Expected & expected) override;
};

}
