#include "cpp_class_grammar.h"
#include "boost/function.hpp"
#include "boost/bind.hpp"
#include "boost/algorithm/string.hpp"

using namespace cpp_grammar;

namespace cpp_grammar {

    template<class _Value>
    Tree<_Value>* Tree<_Value>::newTree(const _Value& value)
    {
        Tree<_Value>* tree = new Tree(NULL);
        tree->m_value = value;
        return tree;
    }

    template<class _Value>
    void Tree<_Value>::addChild(Tree<_Value>* tree)
    {
        tree->m_parent = this;
        m_childlers.push_back(tree);
    }

    template<class _Value>
    void Tree<_Value>::print() {
        std::cout << m_value.c_str() << std::endl;
        std::vector<Tree*>::iterator iter = m_childlers.begin();
        for (; iter != m_childlers.end(); ++iter) {
            (*iter)->print();
        }

    }

    // ����CLASS�ṹ���ض�����Ϊ����
    class class_action {
    public:
        typedef boost::function<void(char)> CallbackOne;
        typedef boost::function<void(const char*, const char*)> CallbackTwo;

        CallbackTwo m_comment;            // ע��
        CallbackTwo m_className;        // ����
        CallbackOne m_innerClass;        // ����
        CallbackOne m_outterClass;        // ����

        CallbackTwo m_funcReturn;        // ������������
        CallbackTwo m_funcName;            // ������
        CallbackTwo m_funcParamType;    // ������������
        CallbackTwo m_funcParamName;    // ������������

        class_action()
            : m_root(NULL)
            , m_node(&m_root)
            , m_newNode(NULL)
        {
            m_comment = boost::bind(&class_action::comment, this, _1, _2);
            m_className = boost::bind(&class_action::className, this, _1, _2);
            m_innerClass = boost::bind(&class_action::innerClass, this, _1);
            m_outterClass = boost::bind(&class_action::outterClass, this, _1);
            m_funcReturn = boost::bind(&class_action::funcReturn, this, _1, _2);
            m_funcName = boost::bind(&class_action::funcName, this, _1, _2);
            m_funcParamType = boost::bind(&class_action::funcParamType, this, _1, _2);
            m_funcParamName = boost::bind(&class_action::funcParamName, this, _1, _2);
        }

        class_tree& getTree() { return m_root; }

    private:
        // ����ע��
        void comment(const char* start, const char* end) {
            std::string s(start, end);
        }
        // ����
        void className(const char* start, const char* end) {
            _class c;
            c.m_className.assign(start, end);
            delete m_newNode;
            m_newNode = m_node->newTree(c);
        }
        // ��/����
        void innerClass(char) { m_node->addChild(m_newNode); m_node = m_newNode; m_newNode = NULL; }
        void outterClass(char) { m_node = m_node->parent(); }
        // ������������
        void funcReturn(const char* start, const char* end) {
            _function f;
            f.m_returnType = ParseType(start, end);
            m_node->m_value.m_funcs.push_back(f);
        }
        // ��������
        void funcName(const char* start, const char* end) {
            m_node->m_value.m_funcs.back().m_funcName.assign(start, end);
        }
        // ������������
        void funcParamType(const char* start, const char* end) {
            _func_param fp;
            fp.m_paramName.assign(start, end);
            m_node->m_value.m_funcs.back().m_funcParams.push_back(fp);
        }
        // ������������
        void funcParamName(const char* start, const char* end) {
            m_node->m_value.m_funcs.back().m_funcParams.back().m_paramType = ParseType(start, end);
        }
        // ���ַ���ת��Ϊ����
        static type ParseType(const char* start, const char* end) {
            std::string s(start, end);
            if (s == "void")
                return t_void;
            if (s == "float")
                return t_float;
            if (s == "double")
                return t_double;
            if (s.find("char") != s.npos && s.find("*") != s.npos)
                return t_c_str;
            return t_undifined;
        }

    private:
        class_tree        m_root;            // �����ڵ�
        class_tree*        m_node;            // ��ǰ���ڽڵ�
        class_tree*        m_newNode;        // ����ӵ��ӽڵ�
    };

    // ɾ������ע��/**/
    struct EraseComment {

        EraseComment(const std::string& source) {
            m_source.reserve(source.size());
            bs::parse_info<> pi = bs::parse(source.c_str(), *(
                bs::comment_p("/*", "*/")[boost::bind(&EraseComment::Comment, this, _1, _2)]
                | bs::anychar_p[boost::bind(&EraseComment::Anychar, this, _1)]
            ));
        }

        const std::string& str() const { return m_source; }

    private:
        static void Comment(EraseComment* self, const char* start, const char* end) {
            std::string s(start, end);
        }
        static void Anychar(EraseComment* self, char ch) {
            self->m_source.push_back(ch);
        }

        mutable std::string m_source;
    };
}

CppClassGrammar::CppClassGrammar()
{
    m_class_action = new class_action();
}

CppClassGrammar::~CppClassGrammar()
{
    delete m_class_action;
    m_class_action = NULL;
}

bool CppClassGrammar::doParse(const std::string& input)
{
    cpp_grammar::EraseComment cmt(input);

    bs::parse_info<> pi = bs::parse(cmt.str().c_str(), *this, bs::space_p);
    return pi.hit;
}

cpp_grammar::class_tree& CppClassGrammar::getTree()
{
    return m_class_action->getTree();
}

template<typename ScannerT>
CppClassGrammar::definition<ScannerT>::definition(const CppClassGrammar& self)
{
    // ����CPP�ؼ��ֺ�����
    _cpp_key = bs::str_p("class");
    _cpp_type = bs::str_p("void") | "int" | "float" | "double"
        | (!bs::str_p("const") >> "char" >> '*');

    /* ��������ע�� */
    _cpp_comment = bs::comment_p("//")[self.m_class_action->m_comment];

    /* ������ʾ�ͷ��ʿ���Ȩ�� */
    // 1.class class_name;                    // ����
    // 2.class class_name {}                // ����
    // 3.class class_name : public base {}    // ����
    // 4.void func();                        // ������
    // 5.void func(int name);                // ����������
    // 6.void func(int, int name);            // ����������
    _identifier = *(~bs::space_p - (bs::ch_p('{') | '(' | ')' | ',' | ';' | ':'));
    _access_ctrl = (bs::str_p("public") | "protected" | "private") >> ':';

    /* ����������(Ŀǰֻ��������) */
    // 1.{ { 123 };; };;
    _tag_brace_nest = (*(bs::anychar_p - '{' - '}'))    // ƥ�������ַ���'{'��'}'Ϊֹ
        >> '{'                                            // ƥ��'{'
        >> *_tag_brace_nest                                // �ݹ�Ƕ��(�����ڿ���Ƕ������)
        >> *~bs::ch_p('}') >> '}'                        // ƥ�������ַ���'}'Ϊֹ��ƥ��'}'
        >> *bs::ch_p(';');                                // ƥ��';'

    /* ������������ */
    // 1.()                    // �޲κ���
    // 2.(int a, int b)        // �вκ����������������ͺͲ������ƣ�
    // 3.(int, int)            // �вκ�����ֻ�����������ͣ�ʡ�Բ������ƣ�
    _func_param = _cpp_type[self.m_class_action->m_funcParamType]
        >> _identifier[self.m_class_action->m_funcParamName]
        >> *(bs::ch_p(',')
        >> _cpp_type[self.m_class_action->m_funcParamType]
        >> _identifier[self.m_class_action->m_funcParamName]);

    /* �������� */
    // 1.void fun() const;
    // 2.void fun() const {}
    _function = _cpp_type[self.m_class_action->m_funcReturn]    // ƥ�亯������ֵ����
        >> _identifier[self.m_class_action->m_funcName]            // ƥ�亯������
        >> '(' >> !_func_param >> ')'                            // ƥ�亯������(ƥ��0�λ�1��)
        >> *(bs::anychar_p - ';' - '{')                            // ƥ�������ַ���';'��'{'Ϊֹ
        >> (bs::ch_p(';') | _tag_brace_nest);                    // ƥ��';'������(����������'{'��'}'��ɵĳɶ��ַ�)

    /*����CLASS*/
    // 1.class test;                    ������
    // 2.class test : public base {};    �ඨ��
    // ��ƥ�䵽�ض����ַ��󣬱��ִ���ض��Ĳ���[]
    _class = bs::str_p("class")                                    // ƥ���ַ���"class"
        >> _identifier[self.m_class_action->m_className]        // ƥ������
        >> *(bs::anychar_p - '{' - ';')                            // ƥ�������ַ���'{'��';'Ϊֹ(ǰ����ʶ���ඨ�壬������ʶ��������)
        >> bs::ch_p('{')[self.m_class_action->m_innerClass]        // ƥ�䵽'{'����ʾ��������
        >> *(_cpp_comment                                        // ƥ�䵥��ע��
            | _access_ctrl                                        // ƥ�����Ȩ��
            | _class                                            // ƥ��Ƕ����(�ݹ�Ƕ�ף�������N��Ƕ���࣬��Ƕ������Ҳ���ܴ���Ƕ����)
            | _function                                            // ƥ�����Ա����
        )
        >> bs::ch_p('}')[self.m_class_action->m_outterClass]    // ƥ��'}'����ʾ��������
        >> *bs::ch_p(';');                                        // �ඨ�����

    /**��������.h�ļ�����һ��.h�ļ��ڣ����ܻᶨ������*/
    _root = *(_cpp_comment | _function | _class | bs::anychar_p);
}