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

    // 解析CLASS结构后特定的行为操作
    class class_action {
    public:
        typedef boost::function<void(char)> CallbackOne;
        typedef boost::function<void(const char*, const char*)> CallbackTwo;

        CallbackTwo m_comment;            // 注释
        CallbackTwo m_className;        // 类名
        CallbackOne m_innerClass;        // 进类
        CallbackOne m_outterClass;        // 出类

        CallbackTwo m_funcReturn;        // 函数返回类型
        CallbackTwo m_funcName;            // 函数名
        CallbackTwo m_funcParamType;    // 函数参数类型
        CallbackTwo m_funcParamName;    // 函数参数名称

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
        // 单行注释
        void comment(const char* start, const char* end) {
            std::string s(start, end);
        }
        // 类名
        void className(const char* start, const char* end) {
            _class c;
            c.m_className.assign(start, end);
            delete m_newNode;
            m_newNode = m_node->newTree(c);
        }
        // 进/出类
        void innerClass(char) { m_node->addChild(m_newNode); m_node = m_newNode; m_newNode = NULL; }
        void outterClass(char) { m_node = m_node->parent(); }
        // 函数返回类型
        void funcReturn(const char* start, const char* end) {
            _function f;
            f.m_returnType = ParseType(start, end);
            m_node->m_value.m_funcs.push_back(f);
        }
        // 函数名称
        void funcName(const char* start, const char* end) {
            m_node->m_value.m_funcs.back().m_funcName.assign(start, end);
        }
        // 函数参数类型
        void funcParamType(const char* start, const char* end) {
            _func_param fp;
            fp.m_paramName.assign(start, end);
            m_node->m_value.m_funcs.back().m_funcParams.push_back(fp);
        }
        // 函数参数名称
        void funcParamName(const char* start, const char* end) {
            m_node->m_value.m_funcs.back().m_funcParams.back().m_paramType = ParseType(start, end);
        }
        // 将字符串转换为类型
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
        class_tree        m_root;            // 树根节点
        class_tree*        m_node;            // 当前所在节点
        class_tree*        m_newNode;        // 新添加的子节点
    };

    // 删除多行注释/**/
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
    // 解析CPP关键字和类型
    _cpp_key = bs::str_p("class");
    _cpp_type = bs::str_p("void") | "int" | "float" | "double"
        | (!bs::str_p("const") >> "char" >> '*');

    /* 解析单行注释 */
    _cpp_comment = bs::comment_p("//")[self.m_class_action->m_comment];

    /* 解析标示和访问控制权限 */
    // 1.class class_name;                    // 类名
    // 2.class class_name {}                // 类名
    // 3.class class_name : public base {}    // 类名
    // 4.void func();                        // 函数名
    // 5.void func(int name);                // 函数参数名
    // 6.void func(int, int name);            // 函数参数名
    _identifier = *(~bs::space_p - (bs::ch_p('{') | '(' | ')' | ',' | ';' | ':'));
    _access_ctrl = (bs::str_p("public") | "protected" | "private") >> ':';

    /* 解析函数体(目前只解析括号) */
    // 1.{ { 123 };; };;
    _tag_brace_nest = (*(bs::anychar_p - '{' - '}'))    // 匹配任意字符到'{'或'}'为止
        >> '{'                                            // 匹配'{'
        >> *_tag_brace_nest                                // 递归嵌套(括号内可以嵌套括号)
        >> *~bs::ch_p('}') >> '}'                        // 匹配任意字符到'}'为止，匹配'}'
        >> *bs::ch_p(';');                                // 匹配';'

    /* 解析函数参数 */
    // 1.()                    // 无参函数
    // 2.(int a, int b)        // 有参函数（包括参数类型和参数名称）
    // 3.(int, int)            // 有参函数（只包括参数类型，省略参数名称）
    _func_param = _cpp_type[self.m_class_action->m_funcParamType]
        >> _identifier[self.m_class_action->m_funcParamName]
        >> *(bs::ch_p(',')
        >> _cpp_type[self.m_class_action->m_funcParamType]
        >> _identifier[self.m_class_action->m_funcParamName]);

    /* 解析函数 */
    // 1.void fun() const;
    // 2.void fun() const {}
    _function = _cpp_type[self.m_class_action->m_funcReturn]    // 匹配函数返回值类型
        >> _identifier[self.m_class_action->m_funcName]            // 匹配函数名称
        >> '(' >> !_func_param >> ')'                            // 匹配函数参数(匹配0次或1次)
        >> *(bs::anychar_p - ';' - '{')                            // 匹配任意字符到';'或'{'为止
        >> (bs::ch_p(';') | _tag_brace_nest);                    // 匹配';'或函数体(函数体是用'{'和'}'组成的成对字符)

    /*解析CLASS*/
    // 1.class test;                    类声明
    // 2.class test : public base {};    类定义
    // 当匹配到特定的字符后，便会执行特定的操作[]
    _class = bs::str_p("class")                                    // 匹配字符串"class"
        >> _identifier[self.m_class_action->m_className]        // 匹配类名
        >> *(bs::anychar_p - '{' - ';')                            // 匹配任意字符到'{'或';'为止(前者是识别类定义，后者是识别类声明)
        >> bs::ch_p('{')[self.m_class_action->m_innerClass]        // 匹配到'{'，表示进类主体
        >> *(_cpp_comment                                        // 匹配单行注释
            | _access_ctrl                                        // 匹配访问权限
            | _class                                            // 匹配嵌套类(递归嵌套，理论上N层嵌套类，既嵌套类中也可能存在嵌套类)
            | _function                                            // 匹配类成员函数
        )
        >> bs::ch_p('}')[self.m_class_action->m_outterClass]    // 匹配'}'，表示出类主体
        >> *bs::ch_p(';');                                        // 类定义结束

    /**解析整个.h文件，在一个.h文件内，可能会定义多个类*/
    _root = *(_cpp_comment | _function | _class | bs::anychar_p);
}