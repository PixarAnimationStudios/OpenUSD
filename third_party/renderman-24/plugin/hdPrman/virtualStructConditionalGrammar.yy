//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
%define api.pure
%error-verbose
%locations
%parse-param { _VSCGParserData * data  }
%lex-param { void * MACRO_YYSCANNER }

%{

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "hdPrman/debugCodes.h"
#include "hdPrman/matfiltResolveVstructs.h"

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"

PXR_NAMESPACE_USING_DIRECTIVE

// ---------------------------------------------------------------------------

struct _VSCGConditionalBase
{
    virtual ~_VSCGConditionalBase() = default;
    
    typedef std::shared_ptr<_VSCGConditionalBase> Ptr;

    virtual bool Eval(const HdMaterialNode2 & node,
            const NdrTokenVec & shaderTypePriority) = 0;
};

// ---------------------------------------------------------------------------

namespace // anonymous
{

struct ConditionalParamBase : public _VSCGConditionalBase
{
    ConditionalParamBase(const TfToken & name)
    : paramName(name)
    {}

    TfToken paramName;
};

struct ConditionalParamIsConnected : public ConditionalParamBase
{
    ConditionalParamIsConnected(const TfToken & name)
    : ConditionalParamBase(name) {}

    bool Eval(const HdMaterialNode2 & node, const NdrTokenVec & shaderTypePriority)
            override {
        
        auto I = node.inputConnections.find(paramName);
        if (I == node.inputConnections.end()) {
            return false;
        }

        return !(*I).second.empty();
    }
};

struct ConditionalParamIsNotConnected : public ConditionalParamBase
{
    ConditionalParamIsNotConnected(const TfToken & name)
    : ConditionalParamBase(name) {}

    bool Eval(const HdMaterialNode2 & node,
            const NdrTokenVec & shaderTypePriority) override {
        auto I = node.inputConnections.find(paramName);
        if (I == node.inputConnections.end()) {
            return true;
        }

        return (*I).second.empty();
    }
};

struct ConditionalParamIsSet : public ConditionalParamBase
{
    ConditionalParamIsSet(const TfToken & name)
    : ConditionalParamBase(name) {}

    bool Eval(const HdMaterialNode2 & node, const NdrTokenVec & shaderTypePriority)
            override {
        return (node.parameters.find(paramName)
                != node.parameters.end());
    }
};

struct ConditionalParamIsNotSet : public ConditionalParamBase
{
    ConditionalParamIsNotSet(const TfToken & name)
    : ConditionalParamBase(name) {}

    bool Eval(const HdMaterialNode2 & node, const NdrTokenVec & shaderTypePriority)
            override {
        return (node.parameters.find(paramName)
                == node.parameters.end());
    }
};

struct ConditionalParamCmpBase : public ConditionalParamBase
{
    ConditionalParamCmpBase(const TfToken & name, const VtValue & v)
    : ConditionalParamBase(name) , value(v) {}

    VtValue value;

    static bool ValueAsNumber(const VtValue v, double * result) {
        if (v.IsHolding<float>()) {
            *result = v.UncheckedGet<float>();
            return true;
        }
        
        if (v.IsHolding<int>()) {
            *result = v.UncheckedGet<int>();
            return true;
        }

        if (v.IsHolding<double>()) {
            *result = v.UncheckedGet<double>();
            return true;
        }

        if (v.IsHolding<bool>()) {
            *result = v.UncheckedGet<bool>();
            return true;
        }

        // TODO check color?

        return false;
    }

    static bool GetParameterValue(
            const HdMaterialNode2 & node,
            const TfToken & paramName,
            const NdrTokenVec & shaderTypePriority,
            VtValue * result) {

        auto I = node.parameters.find(paramName);
        if (I != node.parameters.end()) {
            *result = (*I).second;
            return true;
        }

        // check for a default
        auto& reg = SdrRegistry::GetInstance();

        if (SdrShaderNodeConstPtr sdrNode = reg.GetShaderNodeByIdentifier(
                node.nodeTypeId, shaderTypePriority)) {
            if (NdrPropertyConstPtr ndrProp = sdrNode->GetInput(paramName)) {
                *result = ndrProp->GetDefaultValue();
                return true;
            }
        }

        return false;
    }

    virtual bool CompareNumber(double v1, double v2) = 0;
    virtual bool CompareString(
            const std::string & v1, const std::string & v2) = 0;

    bool Eval(const HdMaterialNode2 & node,
            const NdrTokenVec & shaderTypePriority) override {
        VtValue paramValue;
        if (!GetParameterValue(
                node, paramName, shaderTypePriority, &paramValue)) {
            return false;
        }

        if (value.IsHolding<std::string>()) {
            if (paramValue.IsHolding<std::string>()) {
                return CompareString(value.UncheckedGet<std::string>(),
                        paramValue.UncheckedGet<std::string>());
            }
            return false;
        }

        double d1 = 0, d2 = 0;

        if (ValueAsNumber(paramValue, &d1) && ValueAsNumber(value, &d2)) {    
            
            //std::cerr << d1 << " " << d2 << std::endl;
            return CompareNumber(d1, d2);
        }

        return false;
    }

};


struct ConditionalParamCmpEqualTo : public ConditionalParamCmpBase
{
    ConditionalParamCmpEqualTo(const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {}
    
    bool CompareNumber(double v1, double v2) override {
        return v1 == v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return v1 == v2;
    }
};

struct ConditionalParamCmpNotEqualTo : public ConditionalParamCmpBase
{
    ConditionalParamCmpNotEqualTo(const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {}

    bool CompareNumber(double v1, double v2) override {
        return v1 != v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return v1 != v2;
    }
};


struct ConditionalParamCmpGreaterThan : public ConditionalParamCmpBase
{
    ConditionalParamCmpGreaterThan(const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {
    }

    bool CompareNumber(double v1, double v2) override {
        return v1 > v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return false;
    }
};

struct ConditionalParamCmpLessThan : public ConditionalParamCmpBase
{
    ConditionalParamCmpLessThan(const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {}

    bool CompareNumber(double v1, double v2) override {
        return v1 < v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return false;
    }
};

struct ConditionalParamCmpGreaterThanOrEqualTo
    : public ConditionalParamCmpBase
{
    ConditionalParamCmpGreaterThanOrEqualTo(
            const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {}

    bool CompareNumber(double v1, double v2) override {
        return v1 >= v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return false;
    }
};


struct ConditionalParamCmpLessThanOrEqualTo : public ConditionalParamCmpBase
{
    ConditionalParamCmpLessThanOrEqualTo(
            const TfToken & name, const VtValue & v)
    : ConditionalParamCmpBase(name, v) {}

    bool CompareNumber(double v1, double v2) override {
        return v1 <= v2;
    }

    bool CompareString(const std::string & v1, const std::string & v2)
            override {
        return false;
    }
};

struct ConditionalAnd : _VSCGConditionalBase
{
    ConditionalAnd(_VSCGConditionalBase * left, _VSCGConditionalBase * right)
    : _VSCGConditionalBase(), left(left), right(right) {}

    bool Eval(const HdMaterialNode2 & node,
            const NdrTokenVec & shaderTypePriority) override {

        return (left && left->Eval(node, shaderTypePriority))
                && (right && right->Eval(node, shaderTypePriority)); 

    }

    ~ConditionalAnd() {
        delete left;
        delete right;
    }

    _VSCGConditionalBase * left;
    _VSCGConditionalBase * right;
};

struct ConditionalOr : _VSCGConditionalBase
{
    ConditionalOr(_VSCGConditionalBase * left, _VSCGConditionalBase * right)
    : _VSCGConditionalBase(), left(left), right(right) {}

    ~ConditionalOr() {
        delete left;
        delete right;
    }
    bool Eval(const HdMaterialNode2 & node, const NdrTokenVec & shaderTypePriority)
            override {
        return (left && left->Eval(node, shaderTypePriority))
                || (right && right->Eval(node, shaderTypePriority)); 
    }

    _VSCGConditionalBase * left;
    _VSCGConditionalBase * right;
};


//----------------------------------------------------------------------------


} // namespace anonymous


struct _VSCGValueContainer
{
    _VSCGValueContainer(const VtValue & v)
    : value(v) {}

    VtValue value;
};

struct _VSCGAction {

    enum Action
    {
        Connect,
        Ignore,
        SetConstant,
        CopyParam,
    };

    _VSCGAction(Action action)
    : action(action) {}
    _VSCGAction(Action action, const VtValue & value)
    : action(action), value(value) {}

    Action action;
    VtValue value;
};


struct _VSCGParserData {

    // flex scanner
    void * yyscanner;
    std::string parseError;

    _VSCGAction * action = nullptr;
    _VSCGAction * fallbackAction = nullptr;
    
    std::set<_VSCGConditionalBase *> _intermediateConditions;

    _VSCGConditionalBase * NewCondition(_VSCGConditionalBase * condition) {
        _intermediateConditions.insert(condition);
        return condition;
    }

    _VSCGConditionalBase * ClaimCondition(_VSCGConditionalBase * condition) {
        _intermediateConditions.erase(condition);
        return condition;
    }

    _VSCGConditionalBase * rootCondition = nullptr;

    ~_VSCGParserData() {
        delete action;
        delete fallbackAction;

        for (auto c : _intermediateConditions) {
            delete c;
        }
        _intermediateConditions.clear();
    }
};

PXR_NAMESPACE_OPEN_SCOPE

class MatfiltVstructConditionalEvaluatorImpl
{
public:

    ~MatfiltVstructConditionalEvaluatorImpl() {
        delete condition;
        delete action;
        delete fallbackAction;
    }

    _VSCGConditionalBase * condition = nullptr;
    _VSCGAction * action = nullptr;
    _VSCGAction * fallbackAction = nullptr;
    
};

PXR_NAMESPACE_CLOSE_SCOPE

#include "virtualStructConditionalGrammar.tab.h"
 
%}

%union {
  double number;
  char * string;
  struct _VSCGConditionalBase * condition;
  struct _VSCGAction * action;
  struct _VSCGValueContainer * value;
};

%destructor {
  if ($$)
  {
      free($$);
  }
  $$ = 0;
} <string>

%destructor {
  if ($$)
  {
      delete $$;
  }
  $$ = 0;
} <action>

%destructor {
  if ($$)
  {
      delete $$;
  }
  $$ = 0;
} <value>


%{
int virtualStructConditionalGrammarYylex(
        YYSTYPE * lvalp, YYLTYPE *llocp, void * yyscanner);
void virtualStructConditionalGrammarYyerror(
        YYLTYPE *llocp, _VSCGParserData * data, const char *s);

/*This macro is to hack around the fact that
%lex-param ends up getting cut off, for example:
%lex-param { void * data->yyscanner}
Really ends up passing in yyscanner in the YYLEX macro
*/
#define MACRO_YYSCANNER data->yyscanner
%}
 
%start statement
%token <string> NUMBER
%token <string> STRING
%token <string> PARAM
%token LPAR RPAR
%token OP_EQ OP_NOTEQ
%token OP_GT OP_LT OP_GTEQ OP_LTEQ
%token OP_IS OP_ISNOT
%token OP_AND OP_OR
%token KW_IF KW_ELSE KW_CONNECTED KW_CONNECT KW_IGNORE KW_COPY KW_SET
%token UNRECOGNIZED_TOKEN

%type <value> value
%type <condition> expr statement
%type <action> action


%left OP_OR
%left OP_AND

%%

value : STRING  {
    $$ = new _VSCGValueContainer(VtValue($1));
    free($1);
}
| NUMBER {    
    $$ = new _VSCGValueContainer(VtValue(atof($1)));
    free($1);
}

// op : OP_EQ      {data->curEq = "paramEqualTo";}
//  | OP_NOTEQ     {data->curEq = "paramNotEqualTo";}
//  | OP_GT        {data->curEq = "paramGreaterThan";}
//  | OP_LT        {data->curEq = "paramLessThan";}
//  | OP_GTEQ      {data->curEq = "paramGreaterThanOrEqualTo";}
//  | OP_LTEQ      {data->curEq = "paramLessThanOrEqualTo";}

// TODO, operators
expr: PARAM OP_EQ value {

    $$ = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken($1), $3->value));

    free($1);
    delete $3;
}
| OP_AND OP_EQ value {
    /* PARAM named 'and', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("and"), $3->value));
    delete $3;
}
| OP_OR OP_EQ value {
    /* PARAM named 'or', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("or"), $3->value));
    delete $3;
}
| OP_IS OP_EQ value {
    /* PARAM named 'is', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("is"), $3->value));
    delete $3;
}

| KW_IF OP_EQ value {
    /* PARAM named 'if', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("if"), $3->value));
    delete $3;
}
| KW_ELSE OP_EQ value {
    /* PARAM named 'else', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("else"), $3->value));
    delete $3;
}
| KW_CONNECTED OP_EQ value {
    /* PARAM named 'connected', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("connected"), $3->value));
    delete $3;
}
| KW_CONNECT OP_EQ value {
    /* PARAM named 'connect', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("connect"), $3->value));
    delete $3;
}
| KW_IGNORE OP_EQ value {
        /* PARAM named 'ignore', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("ignore"), $3->value));
    delete $3;
} 
| KW_COPY OP_EQ value {
        /* PARAM named 'copy', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("copy"), $3->value));
    delete $3;
} 
| KW_SET OP_EQ value {
        /* PARAM named 'set', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpEqualTo(TfToken("set"), $3->value));
    delete $3;
}

//--- NOTEQ
| PARAM OP_NOTEQ value {

    $$ = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken($1), $3->value));

    free($1);
    delete $3;
}
| OP_AND OP_NOTEQ value {
    /* PARAM named 'and', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("and"), $3->value));
    delete $3;
}
| OP_OR OP_NOTEQ value {
    /* PARAM named 'or', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("or"), $3->value));
    delete $3;
}
| OP_IS OP_NOTEQ value {
    /* PARAM named 'is', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("is"), $3->value));
    delete $3;
}

| KW_IF OP_NOTEQ value {
    /* PARAM named 'if', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("if"), $3->value));
    delete $3;
}
| KW_ELSE OP_NOTEQ value {
    /* PARAM named 'else', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("else"), $3->value));
    delete $3;
}
| KW_CONNECTED OP_NOTEQ value {
    /* PARAM named 'connected', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(
                    TfToken("connected"), $3->value));
    delete $3;
}
| KW_CONNECT OP_NOTEQ value {
    /* PARAM named 'connect', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("connect"), $3->value));
    delete $3;
}
| KW_IGNORE OP_NOTEQ value {
        /* PARAM named 'ignore', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("ignore"), $3->value));
    delete $3;
} 
| KW_COPY OP_NOTEQ value {
        /* PARAM named 'copy', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("copy"), $3->value));
    delete $3;
} 
| KW_SET OP_NOTEQ value {
        /* PARAM named 'set', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpNotEqualTo(TfToken("set"), $3->value));
    delete $3;
}


//--- GT
| PARAM OP_GT value {

    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken($1), $3->value));

    free($1);
    delete $3;
}
| OP_AND OP_GT value {
    /* PARAM named 'and', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("and"), $3->value));
    delete $3;
}
| OP_OR OP_GT value {
    /* PARAM named 'or', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("or"), $3->value));
    delete $3;
}
| OP_IS OP_GT value {
    /* PARAM named 'is', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("is"), $3->value));
    delete $3;
}

| KW_IF OP_GT value {
    /* PARAM named 'if', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("if"), $3->value));
    delete $3;
}
| KW_ELSE OP_GT value {
    /* PARAM named 'else', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("else"), $3->value));
    delete $3;
}
| KW_CONNECTED OP_GT value {
    /* PARAM named 'connected', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThan(
                    TfToken("connected"), $3->value));
    delete $3;
}
| KW_CONNECT OP_GT value {
    /* PARAM named 'connect', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("connect"), $3->value));
    delete $3;
}
| KW_IGNORE OP_GT value {
        /* PARAM named 'ignore', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("ignore"), $3->value));
    delete $3;
} 
| KW_COPY OP_GT value {
        /* PARAM named 'copy', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("copy"), $3->value));
    delete $3;
} 
| KW_SET OP_GT value {
        /* PARAM named 'set', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThan(TfToken("set"), $3->value));
    delete $3;
}


//--- LT
| PARAM OP_LT value {

    $$ = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken($1), $3->value));

    free($1);
    delete $3;
}
| OP_AND OP_LT value {
    /* PARAM named 'and', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("and"), $3->value));
    delete $3;
}
| OP_OR OP_LT value {
    /* PARAM named 'or', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("or"), $3->value));
    delete $3;
}
| OP_IS OP_LT value {
    /* PARAM named 'is', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("is"), $3->value));
    delete $3;
}

| KW_IF OP_LT value {
    /* PARAM named 'if', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("if"), $3->value));
    delete $3;
}
| KW_ELSE OP_LT value {
    /* PARAM named 'else', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("else"), $3->value));
    delete $3;
}
| KW_CONNECTED OP_LT value {
    /* PARAM named 'connected', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("connected"), $3->value));
    delete $3;
}
| KW_CONNECT OP_LT value {
    /* PARAM named 'connect', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("connect"), $3->value));
    delete $3;
}
| KW_IGNORE OP_LT value {
        /* PARAM named 'ignore', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("ignore"), $3->value));
    delete $3;
} 
| KW_COPY OP_LT value {
        /* PARAM named 'copy', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("copy"), $3->value));
    delete $3;
} 
| KW_SET OP_LT value {
        /* PARAM named 'set', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThan(TfToken("set"), $3->value));
    delete $3;
}

//--- GTEQ
| PARAM OP_GTEQ value {

    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken($1),
                    $3->value));

    free($1);
    delete $3;
}
| OP_AND OP_GTEQ value {
    /* PARAM named 'and', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("and"),
                    $3->value));
    delete $3;
}
| OP_OR OP_GTEQ value {
    /* PARAM named 'or', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("or"),
                    $3->value));
    delete $3;
}
| OP_IS OP_GTEQ value {
    /* PARAM named 'is', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("is"),
                    $3->value));
    delete $3;
}

| KW_IF OP_GTEQ value {
    /* PARAM named 'if', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("if"),
                    $3->value));
    delete $3;
}
| KW_ELSE OP_GTEQ value {
    /* PARAM named 'else', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("else"),
                    $3->value));
    delete $3;
}
| KW_CONNECTED OP_GTEQ value {
    /* PARAM named 'connected', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("connected"),
                    $3->value));
    delete $3;
}
| KW_CONNECT OP_GTEQ value {
    /* PARAM named 'connect', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("connect"),
                    $3->value));
    delete $3;
}
| KW_IGNORE OP_GTEQ value {
        /* PARAM named 'ignore', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("ignore"),
                    $3->value));
    delete $3;
} 
| KW_COPY OP_GTEQ value {
        /* PARAM named 'copy', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("copy"),
                    $3->value));
    delete $3;
} 
| KW_SET OP_GTEQ value {
        /* PARAM named 'set', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpGreaterThanOrEqualTo(TfToken("set"),
                    $3->value));
    delete $3;
}


//--- LTEQ
| PARAM OP_LTEQ value {

    $$ = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken($1),
                    $3->value));

    free($1);
    delete $3;
}
| OP_AND OP_LTEQ value {
    /* PARAM named 'and', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("and"),
                    $3->value));
    delete $3;
}
| OP_OR OP_LTEQ value {
    /* PARAM named 'or', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("or"),
                    $3->value));
    delete $3;
}
| OP_IS OP_LTEQ value {
    /* PARAM named 'is', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("is"),
                    $3->value));
    delete $3;
}

| KW_IF OP_LTEQ value {
    /* PARAM named 'if', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("if"),
                    $3->value));
    delete $3;
}
| KW_ELSE OP_LTEQ value {
    /* PARAM named 'else', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("else"),
                    $3->value));
    delete $3;
}
| KW_CONNECTED OP_LTEQ value {
    /* PARAM named 'connected', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("connected"),
                    $3->value));
    delete $3;
}
| KW_CONNECT OP_LTEQ value {
    /* PARAM named 'connect', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("connect"),
                    $3->value));
    delete $3;
}
| KW_IGNORE OP_LTEQ value {
        /* PARAM named 'ignore', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("ignore"),
                    $3->value));
    delete $3;
} 
| KW_COPY OP_LTEQ value {
        /* PARAM named 'copy', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("copy"),
                    $3->value));
    delete $3;
} 
| KW_SET OP_LTEQ value {
        /* PARAM named 'set', special case */
    $$ = data->NewCondition(
            new ConditionalParamCmpLessThanOrEqualTo(TfToken("set"),
                    $3->value));
    delete $3;
}



//--- ISCONNECTED
| PARAM OP_IS KW_CONNECTED  {

    $$ = data->NewCondition(new ConditionalParamIsConnected(TfToken($1)));
    free($1);
}
| OP_AND OP_IS KW_CONNECTED  {
    /* PARAM named 'and', special case */
    $$ = data->NewCondition(new ConditionalParamIsConnected(TfToken("and")));
}
| OP_OR OP_IS KW_CONNECTED  {
    /* PARAM named 'or', special case */
    $$ = data->NewCondition(new ConditionalParamIsConnected(TfToken("or")));
}
| OP_IS OP_IS KW_CONNECTED  {
    /* PARAM named 'is', special case */
    $$ = data->NewCondition(new ConditionalParamIsConnected(TfToken("is")));
}

| KW_IF OP_IS KW_CONNECTED  {
    /* PARAM named 'if', special case */
    $$ = data->NewCondition(new ConditionalParamIsConnected(TfToken("if")));
}
| KW_ELSE OP_IS KW_CONNECTED  {
    /* PARAM named 'else', special case */
    $$ = data->NewCondition(new ConditionalParamIsConnected(TfToken("else")));
}
| KW_CONNECTED OP_IS KW_CONNECTED  {
    /* PARAM named 'connected', special case */
    $$ = data->NewCondition(new ConditionalParamIsConnected(
            TfToken("connected")));
}
| KW_CONNECT OP_IS KW_CONNECTED  {
    /* PARAM named 'connect', special case */
    $$ = data->NewCondition(new ConditionalParamIsConnected(
            TfToken("connect")));
}
| KW_IGNORE OP_IS KW_CONNECTED  {
        /* PARAM named 'ignore', special case */
    $$ = data->NewCondition(new ConditionalParamIsConnected(
            TfToken("ignore")));
} 
| KW_COPY OP_IS KW_CONNECTED  {
        /* PARAM named 'copy', special case */
    $$ = data->NewCondition(new ConditionalParamIsConnected(TfToken("copy")));
} 
| KW_SET OP_IS KW_CONNECTED  {
        /* PARAM named 'set', special case */
    $$ = data->NewCondition(new ConditionalParamIsConnected(TfToken("set")));
}


//--- IS NOT CONNECTED
| PARAM OP_ISNOT KW_CONNECTED  {

    $$ = data->NewCondition(new ConditionalParamIsNotConnected(TfToken($1)));
    free($1);
}
| OP_AND OP_ISNOT KW_CONNECTED  {
    /* PARAM named 'and', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("and")));
}
| OP_OR OP_ISNOT KW_CONNECTED  {
    /* PARAM named 'or', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("or")));
}
| OP_IS OP_ISNOT KW_CONNECTED  {
    /* PARAM named 'is', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("is")));
}

| KW_IF OP_ISNOT KW_CONNECTED  {
    /* PARAM named 'if', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("if")));
}
| KW_ELSE OP_ISNOT KW_CONNECTED  {
    /* PARAM named 'else', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("else")));
}
| KW_CONNECTED OP_ISNOT KW_CONNECTED  {
    /* PARAM named 'connected', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("connected")));
}
| KW_CONNECT OP_ISNOT KW_CONNECTED  {
    /* PARAM named 'connect', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("connect")));
}
| KW_IGNORE OP_ISNOT KW_CONNECTED  {
        /* PARAM named 'ignore', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("ignore")));
} 
| KW_COPY OP_ISNOT KW_CONNECTED  {
        /* PARAM named 'copy', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("copy")));
} 
| KW_SET OP_ISNOT KW_CONNECTED  {
        /* PARAM named 'set', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotConnected(
            TfToken("set")));
}



//--- IS SET
| PARAM OP_IS KW_SET  {

    $$ = data->NewCondition(new ConditionalParamIsSet(TfToken($1)));
    free($1);
}
| OP_AND OP_IS KW_SET  {
    /* PARAM named 'and', special case */
    $$ = data->NewCondition(new ConditionalParamIsSet(TfToken("and")));
}
| OP_OR OP_IS KW_SET  {
    /* PARAM named 'or', special case */
    $$ = data->NewCondition(new ConditionalParamIsSet(TfToken("or")));
}
| OP_IS OP_IS KW_SET  {
    /* PARAM named 'is', special case */
    $$ = data->NewCondition(new ConditionalParamIsSet(TfToken("is")));
}

| KW_IF OP_IS KW_SET  {
    /* PARAM named 'if', special case */
    $$ = data->NewCondition(new ConditionalParamIsSet(TfToken("if")));
}
| KW_ELSE OP_IS KW_SET  {
    /* PARAM named 'else', special case */
    $$ = data->NewCondition(new ConditionalParamIsSet(TfToken("else")));
}
| KW_CONNECTED OP_IS KW_SET  {
    /* PARAM named 'connected', special case */
    $$ = data->NewCondition(new ConditionalParamIsSet(TfToken("connected")));
}
| KW_CONNECT OP_IS KW_SET  {
    /* PARAM named 'connect', special case */
    $$ = data->NewCondition(new ConditionalParamIsSet(TfToken("connect")));
}
| KW_IGNORE OP_IS KW_SET  {
        /* PARAM named 'ignore', special case */
    $$ = data->NewCondition(new ConditionalParamIsSet(TfToken("ignore")));
} 
| KW_COPY OP_IS KW_SET  {
        /* PARAM named 'copy', special case */
    $$ = data->NewCondition(new ConditionalParamIsSet(TfToken("copy")));
} 
| KW_SET OP_IS KW_SET  {
        /* PARAM named 'set', special case */
    $$ = data->NewCondition(new ConditionalParamIsSet(TfToken("set")));
}


//--- IS NOT SET
| PARAM OP_ISNOT KW_SET  {

    $$ = data->NewCondition(new ConditionalParamIsNotSet(TfToken($1)));
    free($1);
}
| OP_AND OP_ISNOT KW_SET  {
    /* PARAM named 'and', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotSet(TfToken("and")));
}
| OP_OR OP_ISNOT KW_SET  {
    /* PARAM named 'or', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotSet(TfToken("or")));
}
| OP_IS OP_ISNOT KW_SET  {
    /* PARAM named 'is', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotSet(TfToken("is")));
}

| KW_IF OP_ISNOT KW_SET  {
    /* PARAM named 'if', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotSet(TfToken("if")));
}
| KW_ELSE OP_ISNOT KW_SET  {
    /* PARAM named 'else', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotSet(TfToken("else")));
}
| KW_CONNECTED OP_ISNOT KW_SET  {
    /* PARAM named 'connected', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotSet(
            TfToken("connected")));
}
| KW_CONNECT OP_ISNOT KW_SET  {
    /* PARAM named 'connect', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotSet(TfToken("connect")));
}
| KW_IGNORE OP_ISNOT KW_SET  {
        /* PARAM named 'ignore', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotSet(TfToken("ignore")));
} 
| KW_COPY OP_ISNOT KW_SET  {
        /* PARAM named 'copy', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotSet(TfToken("copy")));
} 
| KW_SET OP_ISNOT KW_SET  {
        /* PARAM named 'set', special case */
    $$ = data->NewCondition(new ConditionalParamIsNotSet(TfToken("set")));
}



 | LPAR expr RPAR {
    $$ = $2;
 }
 | expr OP_AND expr {
    $$ = data->NewCondition(new ConditionalAnd(
            data->ClaimCondition($1),
            data->ClaimCondition($3)));
 }
 | expr OP_OR expr {

    $$ = data->NewCondition(new ConditionalOr(
            data->ClaimCondition($1),
            data->ClaimCondition($3))); 
 }
action : KW_COPY PARAM {

    TfToken paramName($2);
    free($2);

    $$ = new _VSCGAction(_VSCGAction::CopyParam, VtValue(paramName));

} 
| KW_CONNECT {
    $$ = new _VSCGAction(_VSCGAction::Connect);
   
}
| KW_IGNORE {
   $$ = new _VSCGAction(_VSCGAction::Ignore);
}
| KW_SET STRING {

    std::string value($2);
    free($2);
    $$ = new _VSCGAction(_VSCGAction::SetConstant, VtValue(value));
}
| KW_SET NUMBER {
    $$ = new _VSCGAction(_VSCGAction::SetConstant, VtValue(atof($2)));
   free($2);
}
    
    
    



statement : action KW_IF expr KW_ELSE action {
    data->action = $1;
    data->rootCondition = data->ClaimCondition($3);
    data->fallbackAction = $5;
 }
 | action KW_IF expr {
    data->action = $1;
    data->rootCondition = data->ClaimCondition($3);

    if ($1->action == _VSCGAction::Ignore) {
        data->fallbackAction = new _VSCGAction(_VSCGAction::Connect);
    } else {
        data->fallbackAction = new _VSCGAction(_VSCGAction::Ignore);
    }
 
 }
 | KW_IF expr KW_ELSE action {
    
    data->action = new _VSCGAction(_VSCGAction::Connect);

    data->rootCondition = data->ClaimCondition($2);


    data->fallbackAction = $4;
    
 }
 | action  {
    data->action = $1;
 }
 | expr {
    data->action = new _VSCGAction(_VSCGAction::Connect);
    data->rootCondition = data->ClaimCondition($1);
    data->fallbackAction = new _VSCGAction(_VSCGAction::Ignore);
 }
 

%%


 

/* Called by yyparse on error */
void virtualStructConditionalGrammarYyerror(
        YYLTYPE *llocp, _VSCGParserData * data, const char *s)
{
    data->parseError = s;
}

typedef struct yy_buffer_state * YY_BUFFER_STATE;
YY_BUFFER_STATE virtualStructConditionalGrammarYy_scan_string(
        const char *, void * );
void virtualStructConditionalGrammarYy_delete_buffer(
        YY_BUFFER_STATE, void * );
int virtualStructConditionalGrammarYylex_init (void ** yyscanner);
int virtualStructConditionalGrammarYylex_destroy (void * yyscanner);



//----------------------------------------------------------------------------

MatfiltVstructConditionalEvaluator::Ptr
MatfiltVstructConditionalEvaluator::Parse(const std::string & inputExpr)
{
    _VSCGParserData data;
    if (virtualStructConditionalGrammarYylex_init(&data.yyscanner)) {
        TF_CODING_ERROR("_VSCGParser: error initializing scanner");
        return nullptr;
    }
    YY_BUFFER_STATE bufferstate =
        virtualStructConditionalGrammarYy_scan_string(
                inputExpr.c_str(), data.yyscanner);
    MatfiltVstructConditionalEvaluator::Ptr result(
            new MatfiltVstructConditionalEvaluator);
    if (virtualStructConditionalGrammarYyparse(&data) == 0) {
        //hintsAttr = data.getHints(prefix, secondaryPrefix);
        
        result->_impl = new MatfiltVstructConditionalEvaluatorImpl;

        result->_impl->condition = data.rootCondition;
        result->_impl->action = data.action;
        result->_impl->fallbackAction = data.fallbackAction;

        data.rootCondition = nullptr;
        data.action = nullptr;
        data.fallbackAction = nullptr;
    } else {
        TF_CODING_ERROR("_VSCGParser: Error parsing '%s'", inputExpr.c_str());
    }
    if (!data.parseError.empty()) {
        TF_CODING_ERROR("_VSCGParser: Error parsing '%s': %s",
                inputExpr.c_str(),
                data.parseError.c_str());
    }

    virtualStructConditionalGrammarYy_delete_buffer(
            bufferstate, data.yyscanner);
    virtualStructConditionalGrammarYylex_destroy(data.yyscanner);

    // TODO
    return result;
}

void MatfiltVstructConditionalEvaluator::Evaluate(
        const SdfPath & nodeId,
        const TfToken & nodeInput,
        const SdfPath & upstreamNodeId,
        const TfToken & upstreamNodeOutput,
        const NdrTokenVec & shaderTypePriority,
        HdMaterialNetwork2 & network) const
{
    if (!_impl) {
        TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: No impl");
        return;
    }

    // Find node
    auto I = network.nodes.find(nodeId);
    if (I == network.nodes.end()) {
        TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: Cannot eval "
                "for node %s; not found in network",
                nodeId.GetText());
        return;
    }
    HdMaterialNode2 & node = (*I).second;

    // if it's already connected explicitly, don't do anything
    if (node.inputConnections.find(nodeInput) != node.inputConnections.end()) {
        return;
    }

    _VSCGAction * chosenAction = nullptr;

    // Get upstream node.
    auto I2 = network.nodes.find(upstreamNodeId);
    if (I2 == network.nodes.end()) {
        // No upstream node; silently ignore
        return;
    }
    const HdMaterialNode2 & upstreamNode = (*I2).second;

    // Decide action to perform.
    if (_impl->condition) {
        if (_impl->condition->Eval(upstreamNode, shaderTypePriority)) {
            chosenAction = _impl->action;
        } else {
            chosenAction = _impl->fallbackAction;
        }            
    } else {
        chosenAction = _impl->action;
    }
    if (!chosenAction) {
        TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: NULL action");
        return;
    }

    // Execute action.
    switch (chosenAction->action) {
    case _VSCGAction::Ignore:
        break;
    case _VSCGAction::Connect:
        node.inputConnections[nodeInput] =
                {{upstreamNodeId, upstreamNodeOutput}};
        break;
    case _VSCGAction::SetConstant:
    {
        // convert the constant to the expected type    
        auto& reg = SdrRegistry::GetInstance();
        SdrShaderNodeConstPtr sdrNode = 
                reg.GetShaderNodeByIdentifier(
                        node.nodeTypeId, shaderTypePriority);
        if (!sdrNode) {
            // TODO, warn
            break;
        }
        NdrPropertyConstPtr ndrProp = sdrNode->GetInput(nodeInput);
        if (!ndrProp) {
            // TODO, warn
            break;
        }
        VtValue value = chosenAction->value;
        TfToken inputType = ndrProp->GetType();
        if (value.IsHolding<std::string>()) {
            if (inputType == SdrPropertyTypes->String) {
                node.parameters[nodeInput] = value;
            } else {
                TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: "
                        "Expected string but found %s\n",
                        inputType.GetText());
                break;
            }
        } else if (value.IsHolding<double>()) {
            // parser always stores numbers as double.
            double doubleValue = value.UncheckedGet<double>();
            VtValue resultValue;
            if (inputType == SdrPropertyTypes->Int) {
                resultValue = VtValue(static_cast<int>(doubleValue));
            } else if (inputType == SdrPropertyTypes->Float) {
                resultValue = VtValue(static_cast<float>(doubleValue));
            }
            if (!resultValue.IsEmpty()) {
                node.parameters[nodeInput] = resultValue;
            } else {
                TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: "
                        "Empty result");
            }
            break;
        } else {
            TF_CODING_ERROR("MatfiltVstructConditionalEvaluator: "
                    "Unhandled type %s\n",
                    value.GetTypeName().c_str());
            break;
        }
        break;
    }
    case _VSCGAction::CopyParam:
    {
        if (chosenAction->value.IsHolding<TfToken>()) {
            const TfToken & copyParamName =
                    chosenAction->value.UncheckedGet<TfToken>();
            // confirm that parameter types are equivalent
            // via Sdr                            
            auto& reg = SdrRegistry::GetInstance();
            SdrShaderNodeConstPtr sdrNode = 
                    reg.GetShaderNodeByIdentifier(
                            node.nodeTypeId, shaderTypePriority);
            SdrShaderNodeConstPtr sdrUpstreamNode = 
                    reg.GetShaderNodeByIdentifier(
                            upstreamNode.nodeTypeId,
                            shaderTypePriority);
            if (!sdrNode || !sdrUpstreamNode) {
                // TODO warn?
                break;
            }
            NdrPropertyConstPtr ndrProp =
                    sdrNode->GetInput(nodeInput);

            NdrPropertyConstPtr ndrUpstreamProp =
                    sdrUpstreamNode->GetInput(copyParamName);

            if (!ndrProp || !ndrUpstreamProp) {
                // TODO warn?
                break;
            }

            // TODO, convert between int and float
            if (ndrProp->GetType() == ndrUpstreamProp->GetType()) {
                auto I = upstreamNode.parameters.find(copyParamName);

                if (I != upstreamNode.parameters.end()) {
                    // authored value
                    node.parameters[nodeInput] = (*I).second;
                } else {
                    // use default
                    node.parameters[nodeInput] =
                            ndrUpstreamProp->GetDefaultValue();
                }
            } else {
                // TODO warn?
            }
        }
        break;
    }
    }
    // TODO
}

MatfiltVstructConditionalEvaluator::~MatfiltVstructConditionalEvaluator() {
    delete _impl;
}
