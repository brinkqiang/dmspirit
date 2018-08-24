#pragma once

#include "boost/spirit/include/classic.hpp"
#include <list>

namespace bs
{
    using namespace boost::spirit;
    using namespace BOOST_SPIRIT_CLASSIC_NS;
}
namespace cpp_grammar {

    // ����
    enum type {
        t_undifined = 0,
        t_void,
        t_int,
        t_float,
        t_double,
        t_c_str,
    };

    // ����������Ϣ
    struct _func_param {
        type m_paramType;            // ��������
        std::string m_paramName;    // ��������
    };

    // ������Ϣ
    struct _function {
        type m_returnType;
        std::string m_funcName;
        std::list<_func_param> m_funcParams;
    };

    // ����Ϣ
    struct _class {
        std::string m_className;
        std::list<_function> m_funcs;
    };

    // ���ṹ
    template<class _Value>
    class Tree {
    public:
        Tree(Tree* parent) : m_parent(parent) {}

        Tree* parent() { return m_parent; }

        Tree* newTree(const _Value& value);
        void addChild(Tree* tree);
        void print();

    public:
        _Value m_value;                    // ��ǰ�ڵ�洢��ֵ
        std::vector<Tree*> m_childlers;    // �ӽڵ��б�

    private:
        Tree* m_parent;                    // ���ڵ�
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

        bs::rule<ScannerT> _cpp_key;            // ��ؼ��֣�class��
        bs::rule<ScannerT> _cpp_type;            // ����
        bs::rule<ScannerT> _cpp_comment;        // ����ע��

        bs::rule<ScannerT> _identifier;            // ��ʾ���������������������������ȣ�
        bs::rule<ScannerT> _access_ctrl;        // ���ʿ���Ȩ�ޣ�public��protected��private��

        bs::rule<ScannerT> _tag_brace_nest;        // �����ڵ�Ƕ�״�����(����������)
        bs::rule<ScannerT> _func_param;            // �������������� + ��������
        bs::rule<ScannerT> _function;            // ����

        bs::rule<ScannerT> _class;                // ��
        bs::rule<ScannerT> _root;

        definition(const CppClassGrammar& self);
        const bs::rule<ScannerT>& start() { return _root; }
    };

    cpp_grammar::class_action* m_class_action;
};