#include "../includes/lexer.h"
namespace Lexer
{
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
        while (pos < isalpha(content[pos]) || content[pos] == '_')
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
            }
        }
        return ret;
    }
    std::string Scanner::read_to_dollar()
    {
        std::string ret;
        int escape_mod = false;
        while (pos < content.size())
        {
            if (content[pos] == '$' && !escape_mod)
            {
                pos++;
                return ret;
            }
            ret += content[pos];
            escape_mod = content[pos] == '\\';
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
                keywords.insert(rules[i].specify_tab.begin(), rules[i].specify_tab.end());
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
                    throw std::runtime_error("LexerGenerator::lex: Lexer Error");
                auto [p, tok] = pos_stac.back();
                // if a symbol is a keyword
                auto val = tok.val;
                if (keywords.count(val))
                    ret.push_back(Token{keywords[val], val});
                else
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
    // end LexerGenerator
}