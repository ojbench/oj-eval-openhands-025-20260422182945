#include <bits/stdc++.h>
// Revision: heuristic transformer + similarity estimator for ACMOJ git submission
using namespace std;

static inline bool isParen(char c){ return c=='(' || c==')'; }
static inline bool isSpace(char c){ return c==' '||c=='\t'||c=='\n'||c=='\r'; }

// Tokenize: split on parentheses and whitespace
static vector<string> tokenize(const string &s){
    vector<string> tokens;
    string cur;
    auto flush=[&](){ if(!cur.empty()){ tokens.push_back(cur); cur.clear(); } };
    for(char c: s){
        if (isParen(c)) { flush(); tokens.emplace_back(1,c); }
        else if (isSpace(c)) { flush(); }
        else { cur.push_back(c); }
    }
    flush();
    return tokens;
}

static bool isNumber(const string &t){ if(t.empty()) return false; char *e=nullptr; 
    // simple check: all digits or leading minus then digits
    size_t i=0; if(t[0]=='-') i=1; if(i>=t.size()) return false; for(; i<t.size(); ++i) if(!isdigit((unsigned char)t[i])) return false; return true; }

static bool isIdentifier(const string &t){ if(t.empty()) return false; if(!isalpha((unsigned char)t[0]) && t[0]!='_') return false; for(char c: t){ if(!(isalnum((unsigned char)c) || c=='_')) return false; } return true; }

static unordered_set<string> reservedKeywords(){
    return {
        "function","main","block","set","print",
        // arithmetic/logical common ops
        "add","sub","mul","div","mod","neg","and","or","not",
        // control
        "if","while","loop","return","call",
        // arrays
        "array.create","array.get","array.set"
    };
}

// Normalize identifiers for anticheat comparison: replace user ids by ID, numbers by NUM
static vector<string> normalize_tokens(const vector<string>& toks){
    auto kw = reservedKeywords();
    vector<string> out; out.reserve(toks.size());
    for(const auto &t: toks){
        if (t=="(" || t==")") { out.push_back(t); continue; }
        if (isNumber(t)) { out.push_back("NUM"); continue; }
        if (kw.count(t)) { out.push_back(t); continue; }
        if (t.find('.')!=string::npos) { out.push_back(t); continue; }
        if (t=="main") { out.push_back(t); continue; }
        if (isIdentifier(t)) { out.push_back("ID"); continue; }
        out.push_back(t);
    }
    return out;
}

static double jaccard_bigram_similarity(const vector<string>& a, const vector<string>& b){
    auto to_bigrams = [](const vector<string>& v){
        vector<pair<string,string>> bg; if(v.size()<2) return bg; bg.reserve(v.size()-1);
        for(size_t i=0;i+1<v.size();++i){ bg.emplace_back(v[i], v[i+1]); }
        return bg;
    };
    auto A=to_bigrams(a), B=to_bigrams(b);
    unordered_map<string,int> ca, cb;
    auto enc = [](const pair<string,string>& p){ return p.first+"\x1f"+p.second; };
    for(auto &p:A) ca[enc(p)]++;
    for(auto &p:B) cb[enc(p)]++;
    long long inter=0, uni=0;
    unordered_set<string> keys; keys.reserve(ca.size()+cb.size());
    for(auto &kv:ca) keys.insert(kv.first);
    for(auto &kv:cb) keys.insert(kv.first);
    for(auto &k:keys){ int x=ca.count(k)?ca[k]:0; int y=cb.count(k)?cb[k]:0; inter += min(x,y); uni += max(x,y); }
    if (uni==0) return 0.0;
    return double(inter)/double(uni);
}

// Pretty print tokens with spaces, one expression per line length limit
static string render_tokens(const vector<string>& toks){
    string out; out.reserve(toks.size()*2);
    int col=0; const int LIM=80; for(size_t i=0;i<toks.size();++i){
        string t=toks[i]; string seg = t; if(i+1<toks.size()) seg += ' ';
        if (col + (int)seg.size() > LIM) { out.push_back('\n'); col=0; }
        out += seg; col += seg.size();
    }
    if(!out.empty() && out.back()==' ') out.pop_back();
    return out;
}

static bool renameable(const string &t, const unordered_set<string>& kw){
    if (t=="main") return false;
    if (kw.count(t)) return false;
    if (t=="(" || t==")") return false;
    if (t.find('.')!=string::npos) return false;
    if (!isIdentifier(t)) return false;
    return true;
}

static string transform_program(const string &prog){
    auto toks = tokenize(prog);
    auto kw = reservedKeywords();
    unordered_map<string,string> mp; int idx=1;
    for(auto &t : toks){
        if (renameable(t, kw)){
            auto it=mp.find(t);
            if(it==mp.end()){
                string nn = string("v") + to_string(idx++);
                mp[t]=nn; t = nn;
            }else{
                t = it->second;
            }
        }
    }
    return render_tokens(toks);
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<string> lines; string line; int end_count=0;
    while (getline(cin, line)){
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line=="endprogram") end_count++;
        lines.push_back(line);
    }

    if (end_count >= 2){
        // Anticheat mode: split into two programs on endprogram
        string p1, p2; int phase=0;
        for(const auto &ln: lines){
            if (ln=="endprogram") { phase++; continue; }
            if (phase==0) { p1 += ln; p1.push_back('\n'); }
            else if (phase==1) { p2 += ln; p2.push_back('\n'); }
        }
        auto t1 = normalize_tokens(tokenize(p1));
        auto t2 = normalize_tokens(tokenize(p2));
        double sim = jaccard_bigram_similarity(t1, t2);
        if (sim < 0) sim = 0; if (sim > 1) sim = 1;
        cout.setf(std::ios::fixed); cout<<setprecision(6)<<sim<<"\n";
        return 0;
    }

    // Cheat mode: transform program text heuristically
    string prog;
    for(size_t i=0;i<lines.size();++i){ prog += lines[i]; prog.push_back('\n'); }
    string out = transform_program(prog);
    cout<<out;
    return 0;
}
