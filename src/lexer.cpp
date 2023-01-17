#include "../includes/lexer.h"
#include <functional>

namespace Utils
{
    std::string read_file(const std::string &filename);
}
namespace Lexer
{
    // Code gen, trivial code
    namespace
    {
        std::string gen_header()
        {
            return "// Following code is generated by program \nnamespace HLex{\n";
        }
        std::string gen_constructor_header(int entry)
        {
            return "Lexer::Lexer(const std::string &con):content(con),entry(" + std::to_string(entry) + "){\n";
        }
        template <typename T>
        std::string conv_to_liter(T t)
        {
            return std::to_string(t);
        }
        template <>
        std::string conv_to_liter(std::string str)
        {
            std::map<char, std::string> escape = {
                {'\n', "\\n"}, {'\t', "\\t"}, {'\r', "\\r"}, {'\\', "\\\\"}, {'\'', "\\'"}};

            std::string ret = "\"";
            for (int i = 0; i < str.size(); i++)
            {
                if (escape.count(str[i]))
                    ret += escape[str[i]];
                else
                    ret += str[i];
            }
            ret += '\"';
            return ret;
        }
        template <>
        std::string conv_to_liter(char ch)
        {
            if (ch == 0)
                return "0";
            std::map<char, std::string> escape = {
                {'\n', "\\n"}, {'\t', "\\t"}, {'\r', "\\r"}, {'\\', "\\\\"}, {'\'', "\\'"}};
            if (escape.count(ch))
                return '\'' + escape[ch] + '\'';
            return '\'' + std::string(1, ch) + '\'';
        }
        template <typename K, typename V>
        std::string gen_tab(const std::map<K, V> &mp)
        {
            if (mp.empty())
                return "{}";
            std::string ret = "{";
            int cnt = 0;
            for (auto [k, v] : mp)
            {
                ret += "{" + conv_to_liter(k) + "," + conv_to_liter(v) + "},";
            }
            ret.back() = '}';
            return ret;
        }
        template <typename K, typename V>
        std::string gen_tab2(const std::map<K, V> &mp, std::function<std::string(V)> f)
        {
            if (mp.empty())
                return "{}";
            std::string ret = "{";
            for (auto [k, v] : mp)
            {
                ret += "{" + conv_to_liter(k) + "," + f(v) + "},";
            }
            ret.back() = '}';
            return ret;
        }
        template <typename V>
        std::string gen_set(const std::set<V> &s)
        {
            if (s.empty())
                return "{}";
            std::string ret = "{";
            for (auto v : s)
                ret += conv_to_liter(v) + ",";
            ret.back() = '}';
            return ret;
        }
        template <typename V>
        std::string gen_vec(const std::vector<V> &s, std::function<std::string(V)> f)
        {
            if (s.empty())
                return "{}";

            std::string ret = "{";
            for (auto v : s)
                ret += f(v) + ",";
            ret.back() = '}';
            return ret;
        }

        std::string gen_func(const std::string &func)
        {
            std::string ret = "[](const std::string &s,int &pos)" + func;
            return ret;
        }
    }

    // Scanner ========================
    void Scanner::match(char ch)
    {
        skip();
        if (pos >= content.size())
            std::runtime_error("Scanner::match() unexpected end!");
        if (content[pos] == ch)
        {
            pos++;
            return;
        }
        std::runtime_error("Scanner::match() invalid char expect " + std::string(1, ch) + " but got" + content[pos]);
    }
    char Scanner::get_ch()
    {
        skip();
        if (pos < content.size())
            return content[pos];
        return 0;
    }
    void Scanner::skip()
    {
        while (pos < content.size() && isspace(content[pos]))
            pos++;
        if (pos < content.size() && content[pos] == '#')
            while (pos < content.size() && content[pos] != '\n')
                pos++;
        if (pos < content.size() && (isspace(content[pos]) || content[pos] == '#'))
            skip();
    }
    std::string Scanner::read_word()
    {
        skip();
        std::string ret;
        while (pos < content.size() && (isalpha(content[pos]) || content[pos] == '_'))
        {
            ret += content[pos];
            pos++;
        }
        return ret;
    }
    RuleLine Scanner::parse_keywords()
    {
        RuleLine ret;
        ret.kind = RuleLine::KEYWORDS;
        match('{');
        while (pos < content.size() && get_ch() != '$')
        {
            std::string symbol = read_word();
            match(':');
            std::string key = read_word();
            ret.specify_tab.insert({key, symbol});
            if (get_ch() != ',')
            {
                match('}');
                break;
            }
            else
                match(',');
        }
        match('$');
        return ret;
    }
    RuleLine Scanner::parse_list()
    {
        RuleLine ret;
        ret.kind = RuleLine::IGNORE;
        match('{');
        while (pos < content.size() && get_ch() != '$')
        {
            std::string item = read_word();
            ret.specify_tab.insert({item, ""});
            if (get_ch() != ',')
            {
                match('}');
                break;
            }
            else
                match(',');
        }
        match('$');
        return ret;
    }
    RuleLine Scanner::parse_user_def()
    {
        RuleLine ret;
        match(',');
        ret.symbol = read_word();
        pos++;
        match(']');

        ret.kind = RuleLine::USER_DEF;
        ret.content = read_to_dollar();
        return ret;
    }
    RuleLine Scanner::next_ruleline()
    {
        RuleLine ret;
        skip();
        auto cur_ch = get_ch();
        if (cur_ch == 0)
        {
            ret.kind = RuleLine::END;
            return ret;
        }
        if (isalpha(cur_ch))
        {
            ret.kind = RuleLine::RE;
            ret.symbol = read_word();
            match(':');
            skip();
            ret.content = read_to_dollar();
        }
        else if (cur_ch = '[')
        {
            match('[');
            std::string keyword = read_word();
            if (keyword == "keywords")
            {
                match(']');
                return parse_keywords();
            }
            else if (keyword == "ignore")
            {
                match(']');
                return parse_list();
            }
            // user define code.
            else if (keyword == "user_def")
                return parse_user_def();
            ReParser::print_line(content, pos);
            throw std::runtime_error("Scanner::next_ruleline() invalid extension!");
        }
        else
            throw std::runtime_error("Scanner::next_ruleline() invalid rule!");
        return ret;
    }
    std::string Scanner::read_to_dollar()
    {
        std::string ret;
        int escape_mod = false;
        while (pos < content.size())
        {
            if (content[pos] == '$')
            {
                pos++;
                return ret;
            }
            ret += content[pos];
            pos++;
        }
        throw std::runtime_error("Scanner::read_to_dollar() $ not found!");
    }
    // end Scanner ======================

    // LexerGenerator
    LexerGenerator::LexerGenerator(Scanner s)
    {
        auto cur_rule = s.next_ruleline();
        while (cur_rule.kind != RuleLine::END)
        {
            rules.push_back(cur_rule);
            cur_rule = s.next_ruleline();
        }
        Alg::Graph *g = nullptr;
        for (int i = 0; i < rules.size(); i++)
        {
            if (rules[i].kind == RuleLine::RE)
            {
                std::cout << "Read RE " << rules[i].symbol << " -- " << rules[i].content << "\n";
                auto cur_graph = ReParser::parser(rules[i].content);
                // attach tag
                cur_graph->traverse_graph([&](Alg::Node *n)
                                          {if(n->is_end) n->val = rules[i].symbol; });

                if (g == nullptr)
                    g = cur_graph;
                else
                    g = Alg::Graph::cup(g, cur_graph);
            }
            else if (rules[i].kind == RuleLine::KEYWORDS)
            {
                std::cout << "Read KEYWORDS\n";

                keywords.insert(rules[i].specify_tab.begin(), rules[i].specify_tab.end());
            }
            else if (rules[i].kind == RuleLine::IGNORE)
            {
                std::cout << "Read IGNORE\n";
                for (auto str : rules[i].specify_tab)
                    ignore.insert(str.first);
            }
            else if (rules[i].kind == RuleLine::USER_DEF)
            {
                std::cout << "Read USERDEF " << rules[i].symbol << "\n";
                user_def.insert({rules[i].symbol, rules[i].content});
            }
            else
                throw std::runtime_error("invalid rule!");
        }
        Alg::SubsetAlg sa(g);
        st = std::move(sa.gen_state_tab().trim_tab());
    }
    std::vector<Token> LexerGenerator::lex(const std::string &str)
    {
        std::vector<Token> ret;
        auto &tab = st.tab;
        int cur_state = st.entry;
        // to roll back state tag val
        std::vector<std::pair<int, Token>> pos_stac;
        // str pos
        int pos = 0;
        std::string cur_token;
        std::string cur_tag;

        while (pos < str.size())
        {
            if (tab[cur_state].count(str[pos]))
            {
                cur_state = tab[cur_state][str[pos]];
                cur_token += str[pos];
                if (st.is_fin(cur_state))
                {
                    if (st.fin_stat_tab[cur_state] != "")
                        cur_tag = st.fin_stat_tab[cur_state];

                    pos_stac.clear();
                    pos_stac.push_back({pos + 1, {cur_tag, cur_token}});
                }
                pos++;
            }
            else
            {
                if (pos_stac.empty())
                {
                    ReParser::print_line(str, pos);
                    throw std::runtime_error(+" -> " + std::string(__FILE__) + ":" + std::to_string(__LINE__) + " LexerGenerator::lex(): Lexer Error");
                }
                auto [p, tok] = pos_stac.back();
                // if a symbol is a keyword pr ignore
                auto val = tok.val;
                if (keywords.count(val))
                    tok = Token{keywords[val], val};

                if (!ignore.count(tok.tag))
                    ret.push_back(tok);

                // roll back
                pos = p;
                cur_state = st.entry;
                cur_token = cur_tag = "";
                pos_stac.clear();
            }
        }
        if (pos_stac.size())
            ret.push_back(pos_stac.back().second);
        return ret;
    }
    std::string LexerGenerator::gen_code(const std::string &temp_path)
    {
        std::string ret = Utils::read_file(temp_path) + "\n" + gen_header() + gen_constructor_header(st.entry);
        ret += "fin_stat_tab = " + gen_tab(st.fin_stat_tab) + ";\n";
        ret += "tab = " + gen_vec<std::map<char, int>>(st.tab, gen_tab<char, int>) + ";\n";
        ret += "ignore = " + gen_set(ignore) + ";\n";
        ret += "keywords = " + gen_tab(keywords) + ";\n";
        ret += "user_defs = " + gen_tab2<std::string, std::string>(user_def, gen_func) + ";\n";

        ret += "}}";
        return ret;
    }
    // end LexerGenerator

}