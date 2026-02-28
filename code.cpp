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

static const vector<Metric> *G_MET = nullptr;
static const vector<int> *G_LEX = nullptr;

struct TeamComp {
    bool operator()(int a, int b) const {
        if (a == b) return false;
        const Metric &A = (*G_MET)[a], &B = (*G_MET)[b];
        if (A.solved != B.solved) return A.solved > B.solved;
        if (A.penalty != B.penalty) return A.penalty < B.penalty;
        for (int i = 0; i < A.tcnt; ++i) {
            if (A.desc_times[i] != B.desc_times[i]) return A.desc_times[i] < B.desc_times[i];
        }
        if ((*G_LEX)[a] != (*G_LEX)[b]) return (*G_LEX)[a] < (*G_LEX)[b];
        return a < b;
    }
};

using RankTree = tree<int, null_type, TeamComp, rb_tree_tag, tree_order_statistics_node_update>;

static inline int status_to_id(const string &s) {
    if (s[0] == 'A') return 0;
    if (s[0] == 'W') return 1;
    if (s[0] == 'R') return 2;
    return 3;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<Team> teams;
    teams.reserve(10000);
    unordered_map<string, int> id_of;
    id_of.reserve(20000);

    vector<int> lex_rank;
    vector<int> rank_of_team;
    vector<Metric> metrics;

    G_MET = &metrics;
    G_LEX = &lex_rank;

    bool started = false, frozen = false;
    int m = 0;

    vector<Submission> all_subs;
    all_subs.reserve(300000);

    RankTree rank_tree, frozen_tree;

    auto insert_solve_time = [&](Metric &mt, int t) {
        int i = mt.tcnt;
        while (i > 0 && mt.desc_times[i - 1] < t) {
            mt.desc_times[i] = mt.desc_times[i - 1];
            --i;
        }
        mt.desc_times[i] = t;
        mt.tcnt++;
    };

    auto refresh_rank_array = [&]() {
        int n = (int)teams.size();
        if ((int)rank_of_team.size() != n) rank_of_team.assign(n, 0);
        int rk = 1;
        for (auto it = rank_tree.begin(); it != rank_tree.end(); ++it, ++rk) {
            rank_of_team[*it] = rk;
        }
    };

    auto rebuild_ranking = [&]() {
        rank_tree.clear();
        frozen_tree.clear();
        int n = (int)teams.size();
        for (int i = 0; i < n; ++i) {
            rank_tree.insert(i);
            if (teams[i].frozen_mask) frozen_tree.insert(i);
        }
        refresh_rank_array();
    };

    auto visible_hidden = [&](int tid, int p) {
        return frozen && ((teams[tid].frozen_mask >> p) & 1U);
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
            int tid = *it;
            cout << teams[tid].name << ' ' << rk << ' ' << metrics[tid].solved << ' ' << metrics[tid].penalty;
            for (int p = 0; p < m; ++p) {
                cout << ' ';
                print_problem_cell(tid, p);
            }
            cout << '\n';
        }
    };

    string cmd;
    while (cin >> cmd) {
        if (cmd == "ADDTEAM") {
            string tname; cin >> tname;
            if (started) cout << "[Error]Add failed: competition has started.\n";
            else if (id_of.count(tname)) cout << "[Error]Add failed: duplicated team name.\n";
            else {
                int tid = (int)teams.size();
                teams.push_back(Team{});
                teams.back().name = tname;
                id_of[tname] = tid;
                cout << "[Info]Add successfully.\n";
            }
        } else if (cmd == "START") {
            string t1, t2; int duration, pcnt;
            cin >> t1 >> duration >> t2 >> pcnt;
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
            rank_tree.clear();
            frozen_tree.clear();
            for (int tid = 0; tid < n; ++tid) rank_tree.insert(tid);
            refresh_rank_array();

            cout << "[Info]Competition starts.\n";
        } else if (cmd == "SUBMIT") {
            char pname; string by_word, tname, with_word, st, at_word; int tim;
            cin >> pname >> by_word >> tname >> with_word >> st >> at_word >> tim;
            int p = pname - 'A', s = status_to_id(st), tid = id_of[tname];

            all_subs.push_back({p, s, tim});
            int idx = (int)all_subs.size() - 1;
            teams[tid].last_idx[p][s] = idx;
            teams[tid].last_idx[p][Team::S_ALL] = idx;
            teams[tid].last_idx[Team::P_ALL][s] = idx;
            teams[tid].last_idx[Team::P_ALL][Team::S_ALL] = idx;

            auto &state = teams[tid].prob[p];
            if (!state.solved) {
                if (s == 0) {
                    state.solved = true;
                    state.solve_time = tim;
                    if (!frozen) {
                        metrics[tid].solved++;
                        metrics[tid].penalty += 20LL * state.wrong + tim;
                        insert_solve_time(metrics[tid], tim);
                    }
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

            rebuild_ranking(); // implicit flush in frozen mode
            print_scoreboard();

            while (!frozen_tree.empty()) {
                int tid = *prev(frozen_tree.end()); // lowest-ranked with frozen problem(s)
                int prob = __builtin_ctz(teams[tid].frozen_mask);

                int old_pos = (int)rank_tree.order_of_key(tid) + 1;
                rank_tree.erase(tid);
                frozen_tree.erase(tid);

                teams[tid].frozen_mask &= ~(1U << prob);
                const auto &ps = teams[tid].prob[prob];
                if (ps.solved) {
                    metrics[tid].solved++;
                    metrics[tid].penalty += 20LL * ps.wrong + ps.solve_time;
                    insert_solve_time(metrics[tid], ps.solve_time);
                }

                int new_pos = (int)rank_tree.order_of_key(tid) + 1;
                if (new_pos < old_pos) {
                    int team2 = *rank_tree.find_by_order(new_pos - 1);
                    cout << teams[tid].name << ' ' << teams[team2].name << ' '
                         << metrics[tid].solved << ' ' << metrics[tid].penalty << '\n';
                }

                rank_tree.insert(tid);
                if (teams[tid].frozen_mask) frozen_tree.insert(tid);
            }

            print_scoreboard();

            frozen = false;
            for (auto &t : teams) t.frozen_mask = 0;
            frozen_tree.clear();
            refresh_rank_array();
        } else if (cmd == "QUERY_RANKING") {
            string tname; cin >> tname;
            if (!id_of.count(tname)) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
                continue;
            }
            cout << "[Info]Complete query ranking.\n";
            if (frozen) {
                cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
            }
            int tid = id_of[tname];
            cout << tname << " NOW AT RANKING " << rank_of_team[tid] << '\n';
        } else if (cmd == "QUERY_SUBMISSION") {
            string tname, where_word, prob_field, and_word, status_field;
            cin >> tname >> where_word >> prob_field >> and_word >> status_field;

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
