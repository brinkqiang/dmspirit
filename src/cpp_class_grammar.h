#pragma once

#include "boost/spirit/include/classic.hpp"
#include <list>

namespace bs
{
    using namespace boost::spirit;
    using namespace BOOST_SPIRIT_CLASSIC_NS;
}
namespace cpp_grammar {

    // 类型
    enum type {
        t_undifined = 0,
        t_void,
        t_int,
        t_float,
        t_double,
        t_c_str,
    };

    // 函数参数信息
    struct _func_param {
        type m_paramType;            // 参数类型
        std::string m_paramName;    // 参数名称
    };

    // 函数信息
    struct _function {
        type m_returnType;
        std::string m_funcName;
        std::list<_func_param> m_funcParams;
    };

    // 类信息
    struct _class {
        std::string m_className;
        std::list<_function> m_funcs;
    };

    // 树结构
    template<class _Value>
    class Tree {
    public:
        Tree(Tree* parent) : m_parent(parent) {}

        Tree* parent() { return m_parent; }

        Tree* newTree(const _Value& value);
        void addChild(Tree* tree);
        void print();

    public:
        _Value m_value;                    // 当前节点存储的值
        std::vector<Tree*> m_childlers;    // 子节点列表

    private:
        Tree* m_parent;                    // 父节点
    };
    typedef Tree<_class> class_tree;

    class class_action;
}

struct CppClassGrammar : public bs::grammar < CppClassGrammar > {

    CppClassGrammar();
    ~CppClassGrammar();

    cpp_grammar::class_tree& getTree();

    bool doParse(const std::string& input);

    template<typename ScannerT>
    struct definition {

        bs::rule<ScannerT> _cpp_key;            // 类关键字（class）
        bs::rule<ScannerT> _cpp_type;            // 类型
        bs::rule<ScannerT> _cpp_comment;        // 单行注释

        bs::rule<ScannerT> _identifier;            // 标示（解析类名、函数名、参数名等）
        bs::rule<ScannerT> _access_ctrl;        // 访问控制权限（public、protected、private）

        bs::rule<ScannerT> _tag_brace_nest;        // 函数内的嵌套大括号(包括函数体)
        bs::rule<ScannerT> _func_param;            // 函数参数（类型 + 参数名）
        bs::rule<ScannerT> _function;            // 函数

        bs::rule<ScannerT> _class;                // 类
        bs::rule<ScannerT> _root;

        definition(const CppClassGrammar& self);
        const bs::rule<ScannerT>& start() { return _root; }
    };

    cpp_grammar::class_action* m_class_action;
};