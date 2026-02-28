#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>

using namespace std;
using namespace __gnu_pbds;

struct Submission { int problem, status, time; };
struct ProblemState { int wrong = 0; bool solved = false; int solve_time = 0; };

struct Team {
    string name;
    vector<ProblemState> prob;
    vector<int> wrong_at_freeze, hidden_cnt;
    vector<char> unsolved_at_freeze;
    uint32_t frozen_mask = 0;

    static constexpr int P_ALL = 26;
    static constexpr int S_ALL = 4;
    int last_idx[27][5];

    Team() { for (auto &r : last_idx) for (int &x : r) x = -1; }
};

struct Metric {
    int solved = 0;
    long long penalty = 0;
    array<int, 26> desc_times{};
    int tcnt = 0;
};

struct Key {
    int tid = -1;
    int lex = -1;
    int solved = 0;
    long long penalty = 0;
    array<int, 26> desc_times{};
    int tcnt = 0;
};

struct KeyComp {
    bool operator()(const Key &a, const Key &b) const {
        if (a.solved != b.solved) return a.solved > b.solved;
        if (a.penalty != b.penalty) return a.penalty < b.penalty;
        for (int i = 0; i < a.tcnt; ++i) {
            if (a.desc_times[i] != b.desc_times[i]) return a.desc_times[i] < b.desc_times[i];
        }
        if (a.lex != b.lex) return a.lex < b.lex;
        return a.tid < b.tid;
    }
};

using RankTree = tree<Key, null_type, KeyComp, rb_tree_tag, tree_order_statistics_node_update>;

static int status_to_id(const string &s) {
    if (s == "Accepted") return 0;
    if (s == "Wrong_Answer") return 1;
    if (s == "Runtime_Error") return 2;
    return 3;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<Team> teams;
    teams.reserve(10000);
    unordered_map<string, int> id_of;
    id_of.reserve(20000);

    vector<int> lex_rank; // tid -> lexicographic rank among team names

    bool started = false, frozen = false;
    int m = 0;

    vector<Submission> all_subs;
    all_subs.reserve(300000);

    vector<Metric> metrics;
    vector<Key> cur_key;
    RankTree rank_tree, frozen_tree;

    auto visible_hidden = [&](int tid, int p) {
        return frozen && ((teams[tid].frozen_mask >> p) & 1U);
    };

    auto calc_metric = [&](int tid) {
        Metric mt;
        for (int p = 0; p < m; ++p) {
            if (visible_hidden(tid, p)) continue;
            const auto &ps = teams[tid].prob[p];
            if (ps.solved) {
                mt.solved++;
                mt.penalty += 20LL * ps.wrong + ps.solve_time;
                mt.desc_times[mt.tcnt++] = ps.solve_time;
            }
        }
        sort(mt.desc_times.begin(), mt.desc_times.begin() + mt.tcnt, greater<int>());
        return mt;
    };

    auto make_key = [&](int tid) {
        Key k;
        k.tid = tid;
        k.lex = lex_rank[tid];
        k.solved = metrics[tid].solved;
        k.penalty = metrics[tid].penalty;
        k.tcnt = metrics[tid].tcnt;
        k.desc_times = metrics[tid].desc_times;
        return k;
    };

    auto rebuild_ranking = [&]() {
        rank_tree.clear();
        frozen_tree.clear();
        int n = (int)teams.size();
        if ((int)metrics.size() != n) metrics.resize(n);
        if ((int)cur_key.size() != n) cur_key.resize(n);
        for (int i = 0; i < n; ++i) {
            metrics[i] = calc_metric(i);
            cur_key[i] = make_key(i);
            rank_tree.insert(cur_key[i]);
            if (teams[i].frozen_mask) frozen_tree.insert(cur_key[i]);
        }
    };

    auto print_problem_cell = [&](int tid, int p) {
        if (visible_hidden(tid, p)) {
            int x = teams[tid].wrong_at_freeze[p], y = teams[tid].hidden_cnt[p];
            if (x == 0) cout << "0/" << y;
            else cout << '-' << x << '/' << y;
            return;
        }
        const auto &ps = teams[tid].prob[p];
        if (ps.solved) {
            if (ps.wrong == 0) cout << '+';
            else cout << '+' << ps.wrong;
        } else {
            if (ps.wrong == 0) cout << '.';
            else cout << '-' << ps.wrong;
        }
    };

    auto print_scoreboard = [&]() {
        int rk = 1;
        for (auto it = rank_tree.begin(); it != rank_tree.end(); ++it, ++rk) {
            int tid = it->tid;
            cout << teams[tid].name << ' ' << rk << ' ' << metrics[tid].solved << ' ' << metrics[tid].penalty;
            for (int p = 0; p < m; ++p) {
                cout << ' ';
                print_problem_cell(tid, p);
            }
            cout << '\n';
        }
    };

    auto ranking_of = [&](int tid) {
        return (int)rank_tree.order_of_key(cur_key[tid]) + 1;
    };

    string line;
    while (getline(cin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string cmd;
        ss >> cmd;

        if (cmd == "ADDTEAM") {
            string tname; ss >> tname;
            if (started) {
                cout << "[Error]Add failed: competition has started.\n";
            } else if (id_of.count(tname)) {
                cout << "[Error]Add failed: duplicated team name.\n";
            } else {
                int tid = (int)teams.size();
                teams.push_back(Team{});
                teams.back().name = tname;
                id_of[tname] = tid;
                cout << "[Info]Add successfully.\n";
            }
        } else if (cmd == "START") {
            string tmp; int duration, pcnt;
            ss >> tmp >> duration >> tmp >> pcnt;
            (void)duration;
            if (started) {
                cout << "[Error]Start failed: competition has started.\n";
                continue;
            }
            started = true;
            m = pcnt;

            int n = (int)teams.size();
            vector<pair<string,int>> names;
            names.reserve(n);
            for (int i = 0; i < n; ++i) names.push_back({teams[i].name, i});
            sort(names.begin(), names.end());
            lex_rank.assign(n, 0);
            for (int i = 0; i < n; ++i) lex_rank[names[i].second] = i;

            for (auto &t : teams) {
                t.prob.assign(m, ProblemState{});
                t.wrong_at_freeze.assign(m, 0);
                t.hidden_cnt.assign(m, 0);
                t.unsolved_at_freeze.assign(m, 0);
                t.frozen_mask = 0;
            }

            metrics.assign(n, Metric{});
            cur_key.assign(n, Key{});

            // Before first FLUSH, ranking is by lexicographic team name.
            rank_tree.clear();
            frozen_tree.clear();
            for (int i = 0; i < n; ++i) {
                cur_key[i] = Key{i, lex_rank[i], 0, 0, {}, 0};
                rank_tree.insert(cur_key[i]);
            }

            cout << "[Info]Competition starts.\n";
        } else if (cmd == "SUBMIT") {
            char pname; string by_word, tname, with_word, st, at_word; int tim;
            ss >> pname >> by_word >> tname >> with_word >> st >> at_word >> tim;
            int p = pname - 'A';
            int s = status_to_id(st);
            int tid = id_of[tname];

            all_subs.push_back({p, s, tim});
            int idx = (int)all_subs.size() - 1;
            int ps[2] = {p, Team::P_ALL};
            int ssid[2] = {s, Team::S_ALL};
            for (int pi : ps) for (int si : ssid) teams[tid].last_idx[pi][si] = idx;

            auto &state = teams[tid].prob[p];
            if (!state.solved) {
                if (s == 0) {
                    state.solved = true;
                    state.solve_time = tim;
                } else {
                    state.wrong++;
                }
            }

            if (frozen && teams[tid].unsolved_at_freeze[p]) {
                teams[tid].hidden_cnt[p]++;
                teams[tid].frozen_mask |= (1U << p);
            }
        } else if (cmd == "FLUSH") {
            cout << "[Info]Flush scoreboard.\n";
            rebuild_ranking();
        } else if (cmd == "FREEZE") {
            if (frozen) {
                cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
                continue;
            }
            frozen = true;
            for (auto &t : teams) {
                t.frozen_mask = 0;
                for (int p = 0; p < m; ++p) {
                    t.hidden_cnt[p] = 0;
                    t.unsolved_at_freeze[p] = t.prob[p].solved ? 0 : 1;
                    t.wrong_at_freeze[p] = t.prob[p].wrong;
                }
            }
            frozen_tree.clear();
            cout << "[Info]Freeze scoreboard.\n";
        } else if (cmd == "SCROLL") {
            if (!frozen) {
                cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
                continue;
            }
            cout << "[Info]Scroll scoreboard.\n";

            rebuild_ranking(); // flush first under frozen visibility
            print_scoreboard();

            while (!frozen_tree.empty()) {
                int tid = prev(frozen_tree.end())->tid; // lowest ranked with frozen problems
                int prob = __builtin_ctz(teams[tid].frozen_mask);

                Key oldk = cur_key[tid];
                int old_pos = (int)rank_tree.order_of_key(oldk) + 1;

                rank_tree.erase(oldk);
                frozen_tree.erase(oldk);

                teams[tid].frozen_mask &= ~(1U << prob);
                metrics[tid] = calc_metric(tid);
                Key newk = make_key(tid);

                int new_pos = (int)rank_tree.order_of_key(newk) + 1;
                if (new_pos < old_pos) {
                    int team2 = rank_tree.find_by_order(new_pos - 1)->tid;
                    cout << teams[tid].name << ' ' << teams[team2].name << ' '
                         << metrics[tid].solved << ' ' << metrics[tid].penalty << '\n';
                }

                rank_tree.insert(newk);
                cur_key[tid] = newk;
                if (teams[tid].frozen_mask) frozen_tree.insert(newk);
            }

            print_scoreboard();

            frozen = false;
            for (auto &t : teams) t.frozen_mask = 0;
            frozen_tree.clear();
        } else if (cmd == "QUERY_RANKING") {
            string tname; ss >> tname;
            if (!id_of.count(tname)) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
                continue;
            }
            cout << "[Info]Complete query ranking.\n";
            if (frozen) {
                cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
            }
            int tid = id_of[tname];
            cout << tname << " NOW AT RANKING " << ranking_of(tid) << '\n';
        } else if (cmd == "QUERY_SUBMISSION") {
            string tname; ss >> tname;
            string where_word, prob_field, and_word, status_field;
            ss >> where_word >> prob_field >> and_word >> status_field;

            if (!id_of.count(tname)) {
                cout << "[Error]Query submission failed: cannot find the team.\n";
                continue;
            }

            string prob_val = prob_field.substr(prob_field.find('=') + 1);
            string stat_val = status_field.substr(status_field.find('=') + 1);
            int pidx = (prob_val == "ALL") ? Team::P_ALL : (prob_val[0] - 'A');
            int sidx = (stat_val == "ALL") ? Team::S_ALL : status_to_id(stat_val);

            cout << "[Info]Complete query submission.\n";
            int tid = id_of[tname];
            int idx = teams[tid].last_idx[pidx][sidx];
            if (idx == -1) {
                cout << "Cannot find any submission.\n";
            } else {
                static const char *status_name[4] = {
                    "Accepted", "Wrong_Answer", "Runtime_Error", "Time_Limit_Exceed"
                };
                const auto &sub = all_subs[idx];
                cout << tname << ' ' << char('A' + sub.problem) << ' '
                     << status_name[sub.status] << ' ' << sub.time << '\n';
            }
        } else if (cmd == "END") {
            cout << "[Info]Competition ends.\n";
            break;
        }
    }

    return 0;
}
