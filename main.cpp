/*
 * SettleUp Expense Manager
 * DSA Project
 *
 * Data Structures Used:
 * - Graph (Adjacency Matrix)
 * - Priority Queue (Heap)
 * - Multiset
 *
 * Compile:
 * g++ main.cpp -o SettleUp
 * .\settleup.exe
 */


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <set>
#include <tuple>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// ---------------------- ANSI color codes ----------------------
namespace Color {
    const string RESET   = "\033[0m";
    const string BOLD    = "\033[1m";
    const string RED     = "\033[31m";
    const string GREEN   = "\033[32m";
    const string YELLOW  = "\033[33m";
    const string BLUE    = "\033[34m";
    const string MAGENTA = "\033[35m";
    const string CYAN    = "\033[36m";
    const string GRAY    = "\033[90m";
}

// Enables ANSI escape code interpretation on Windows consoles.
// On Linux/Mac terminals this is unnecessary (they support ANSI by
// default), so the function is a no-op there.
void enableAnsiColors() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

string currentTimestamp() {
    time_t now = time(nullptr);
    tm ltm{};
#ifdef _WIN32
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    ostringstream oss;
    oss << put_time(&ltm, "%H:%M:%S");
    return oss.str();
}

// A single settlement instruction: "from" pays "to" the given amount.
struct Transaction {
    string from;
    string to;
    long long amountPaise; // stored in paise/cents to avoid floating point drift

    // Ordering for the multiset: largest transactions first, tie-break by name
    bool operator<(const Transaction& other) const {
        if (amountPaise != other.amountPaise)
            return amountPaise > other.amountPaise;
        if (from != other.from) return from < other.from;
        return to < other.to;
    }
};

// A single entry in the transaction history log.
struct HistoryEntry {
    string timestamp;
    string description;
};

class ExpenseManager {
private:
    vector<string> people;                 // index -> name
    unordered_map<string, int> indexOf;    // name -> index
    vector<vector<long long>> graph;       // graph[i][j] = amount i directly owes j (paise)
    vector<HistoryEntry> history;          // log of every action taken

    static long long toPaise(double rupees) {
        return llround(rupees * 100.0);
    }

    static string toRupeeString(long long paise) {
        bool neg = paise < 0;
        paise = abs(paise);
        long long rupees = paise / 100;
        long long cents = paise % 100;
        ostringstream oss;
        oss << (neg ? "-" : "") << rupees << "." << setw(2) << setfill('0') << cents;
        return oss.str();
    }

    void ensureCapacity(int newSize) {
        for (auto& row : graph) row.resize(newSize, 0);
        graph.resize(newSize, vector<long long>(newSize, 0));
    }

    void logEvent(const string& description) {
        history.push_back({currentTimestamp(), description});
    }

public:
    int getOrAddPerson(const string& name) {
        auto it = indexOf.find(name);
        if (it != indexOf.end()) return it->second;
        int idx = (int)people.size();
        indexOf[name] = idx;
        people.push_back(name);
        ensureCapacity(idx + 1);
        logEvent("Added person: " + name);
        return idx;
    }

    bool personExists(const string& name) const { return indexOf.find(name) != indexOf.end(); }

    bool hasPeople() const { return !people.empty(); }
    int personCount() const { return (int)people.size(); }
    const vector<string>& getPeople() const { return people; }
    const vector<HistoryEntry>& getHistory() const { return history; }

    void addExpense(const string& payer, const vector<string>& participants, double amountRupees) {
        if (participants.empty()) {
            cout << Color::YELLOW << "  [!] No participants given, expense skipped.\n" << Color::RESET;
            return;
        }
        if (amountRupees <= 0.0) {
            cout << Color::YELLOW << "  [!] Expense amount must be greater than zero, skipped.\n" << Color::RESET;
            return;
        }

        // Remove duplicate participant names (keep first occurrence, warn on skip)
        vector<string> uniqueParticipants;
        set<string> seen;
        for (const auto& name : participants) {
            if (seen.insert(name).second) {
                uniqueParticipants.push_back(name);
            } else {
                cout << Color::YELLOW << "  [!] Duplicate participant \"" << name << "\" ignored.\n" << Color::RESET;
            }
        }

        int payerIdx = getOrAddPerson(payer);
        long long totalPaise = toPaise(amountRupees);
        const vector<string>& participantsList = uniqueParticipants;
        int n = (int)participantsList.size();
        long long basePaise = totalPaise / n;
        long long remainder = totalPaise % n;

        for (int i = 0; i < n; i++) {
            int pIdx = getOrAddPerson(participantsList[i]);
            if (pIdx == payerIdx) continue;
            long long share = basePaise + (i < remainder ? 1 : 0);
            graph[pIdx][payerIdx] += share;
        }

        ostringstream desc;
        desc << payer << " paid Rs " << toRupeeString(totalPaise) << " split among " << n << " people";
        logEvent(desc.str());
    }

    void addDirectDebt(const string& debtor, const string& creditor, double amountRupees) {
        if (amountRupees <= 0.0) {
            cout << Color::YELLOW << "  [!] Debt amount must be greater than zero, skipped.\n" << Color::RESET;
            return;
        }
        int d = getOrAddPerson(debtor);
        int c = getOrAddPerson(creditor);
        if (d == c) {
            cout << Color::YELLOW << "  [!] A person cannot owe themselves, skipped.\n" << Color::RESET;
            return;
        }
        long long paise = toPaise(amountRupees);
        graph[d][c] += paise;
        logEvent(debtor + " owes " + creditor + " Rs " + toRupeeString(paise) + " (direct debt)");
    }

    vector<long long> computeNetBalances() const {
        int n = personCount();
        vector<long long> net(n, 0);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                net[j] += graph[i][j];
                net[i] -= graph[i][j];
            }
        }
        return net;
    }

    void showBalances() {
        if (!hasPeople()) {
            cout << Color::YELLOW << "  No people added yet.\n" << Color::RESET;
            return;
        }
        vector<long long> net = computeNetBalances();
        cout << "\n  " << Color::BOLD << Color::CYAN << "---- Net Balances ----" << Color::RESET << "\n";
        for (int i = 0; i < personCount(); i++) {
            cout << "  " << left << setw(15) << people[i];
            if (net[i] > 0)
                cout << Color::GREEN << "should receive Rs " << toRupeeString(net[i]) << Color::RESET << "\n";
            else if (net[i] < 0)
                cout << Color::RED << "owes Rs " << toRupeeString(-net[i]) << Color::RESET << "\n";
            else
                cout << Color::GRAY << "is settled up" << Color::RESET << "\n";
        }
        cout << "  " << Color::CYAN << "-----------------------" << Color::RESET << "\n";
        logEvent("Viewed net balances");
    }

    vector<Transaction> minimizeCashFlow() const {
        vector<long long> net = computeNetBalances();

        priority_queue<pair<long long, int>> creditors;
        priority_queue<pair<long long, int>> debtors;

        for (int i = 0; i < (int)net.size(); i++) {
            if (net[i] > 0) creditors.push({net[i], i});
            else if (net[i] < 0) debtors.push({-net[i], i});
        }

        vector<Transaction> settlements;

        while (!creditors.empty() && !debtors.empty()) {
            auto [creditAmt, creditIdx] = creditors.top(); creditors.pop();
            auto [debtAmt, debtIdx] = debtors.top(); debtors.pop();

            long long settleAmt = min(creditAmt, debtAmt);
            settlements.push_back({people[debtIdx], people[creditIdx], settleAmt});

            long long creditRemain = creditAmt - settleAmt;
            long long debtRemain = debtAmt - settleAmt;

            if (creditRemain > 0) creditors.push({creditRemain, creditIdx});
            if (debtRemain > 0) debtors.push({debtRemain, debtIdx});
        }

        return settlements;
    }

    void showMinimizedSettlements() {
        if (!hasPeople()) {
            cout << Color::YELLOW << "  No people added yet.\n" << Color::RESET;
            return;
        }
        vector<Transaction> settlements = minimizeCashFlow();

        if (settlements.empty()) {
            cout << "\n  " << Color::GREEN << "Everyone is already settled up! No transactions needed." << Color::RESET << "\n";
            logEvent("Computed settlement plan: everyone already settled up");
            return;
        }

        multiset<Transaction> sortedSettlements(settlements.begin(), settlements.end());

        cout << "\n  " << Color::BOLD << Color::MAGENTA
             << "==== Minimum Transactions to Settle Up (" << sortedSettlements.size() << " total) ====" << Color::RESET << "\n";
        int i = 1;
        for (const auto& t : sortedSettlements) {
            cout << "  " << i++ << ". " << Color::RED << t.from << Color::RESET
                 << " pays " << Color::GREEN << t.to << Color::RESET
                 << "  Rs " << Color::BOLD << toRupeeString(t.amountPaise) << Color::RESET << "\n";
        }
        cout << "  " << Color::MAGENTA << "=================================================" << Color::RESET << "\n";

        logEvent("Computed settlement plan: " + to_string(sortedSettlements.size()) + " transaction(s)");
    }

    void showHistory() const {
        if (history.empty()) {
            cout << Color::YELLOW << "  No history yet.\n" << Color::RESET;
            return;
        }
        cout << "\n  " << Color::BOLD << Color::CYAN << "---- Transaction History Log ----" << Color::RESET << "\n";
        for (size_t i = 0; i < history.size(); i++) {
            cout << "  " << Color::GRAY << "[" << history[i].timestamp << "] " << Color::RESET
                 << history[i].description << "\n";
        }
        cout << "  " << Color::CYAN << "----------------------------------" << Color::RESET << "\n";
    }

    bool exportHistory(const string& filename) const {
        ofstream out(filename);
        if (!out.is_open()) return false;
        out << "SettleUp - Transaction History Log\n";
        out << "====================================\n";
        for (const auto& h : history) {
            out << "[" << h.timestamp << "] " << h.description << "\n";
        }
        out.close();
        return true;
    }
};

// ---------------------- Console helpers ----------------------

int readInt(const string& prompt) {
    int x;
    while (true) {
        cout << Color::CYAN << prompt << Color::RESET;
        if (cin >> x) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard rest of line
            return x;
        }
        cout << Color::RED << "  [!] Please enter a valid integer.\n" << Color::RESET;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

double readDouble(const string& prompt) {
    double x;
    while (true) {
        cout << Color::CYAN << prompt << Color::RESET;
        if (cin >> x) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard rest of line
            return x;
        }
        cout << Color::RED << "  [!] Please enter a valid amount.\n" << Color::RESET;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

// Trims leading/trailing whitespace from a string.
static string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Reads a full line so names with spaces (e.g. "Mary Jane") are supported.
// Re-prompts if the trimmed input is empty.
string readWord(const string& prompt) {
    while (true) {
        cout << Color::CYAN << prompt << Color::RESET;
        string s;
        getline(cin, s);
        s = trim(s);
        if (!s.empty()) return s;
        cout << Color::RED << "  [!] Name cannot be empty.\n" << Color::RESET;
    }
}

void printMenu() {
    cout << "\n" << Color::BOLD << Color::BLUE
         << "==================== SettleUp Expense Manager ====================" << Color::RESET << "\n"
         << " 1. Add a person\n"
         << " 2. Add a shared expense (split equally among participants)\n"
         << " 3. Add a direct debt (X owes Y a specific amount)\n"
         << " 4. Show current net balances\n"
         << " 5. Compute & show minimum settlement transactions\n"
         << " 6. Show transaction history log\n"
         << " 7. Export history log to a text file\n"
         << " 8. Exit\n"
         << Color::BOLD << Color::BLUE
         << "====================================================================" << Color::RESET << "\n";
}

int main() {
    enableAnsiColors();

    cout << "\n" << Color::BOLD << Color::MAGENTA
         << "Welcome to SettleUp - Expense Manager" << Color::RESET << "\n"
         << "Minimum Cash Flow settlement using Graphs, Heaps & Multisets\n";

    ExpenseManager manager;
    bool running = true;

    while (running) {
        printMenu();
        int choice = readInt("Enter your choice: ");

        switch (choice) {
            case 1: {
                string name = readWord("  Enter person's full name: ");
                if (manager.personExists(name)) {
                    cout << Color::YELLOW << "  Person already exists" << Color::RESET << "\n";
                } else {
                    manager.getOrAddPerson(name);
                    cout << Color::GREEN << "  Added: " << name << Color::RESET << "\n";
                }
                break;
            }
            case 2: {
                string payer = readWord("  Who paid? ");
                int n = readInt("  How many people share this expense (including payer if they share too)? ");
                vector<string> participants;
                participants.reserve(n);
                for (int i = 0; i < n; i++) {
                    string p = readWord("    Participant " + to_string(i + 1) + " name: ");
                    participants.push_back(p);
                }
                double amount = readDouble("  Total amount paid (Rs): ");
                manager.addExpense(payer, participants, amount);
                cout << Color::GREEN << "  Expense recorded.\n" << Color::RESET;
                break;
            }
            case 3: {
                string debtor = readWord("  Who owes money (debtor)? ");
                string creditor = readWord("  Who is owed money (creditor)? ");
                double amount = readDouble("  Amount (Rs): ");
                manager.addDirectDebt(debtor, creditor, amount);
                cout << Color::GREEN << "  Direct debt recorded.\n" << Color::RESET;
                break;
            }
            case 4:
                manager.showBalances();
                break;
            case 5:
                manager.showMinimizedSettlements();
                break;
            case 6:
                manager.showHistory();
                break;
            case 7: {
                string filename = readWord("  Enter filename to export to (e.g. history.txt): ");
                if (manager.exportHistory(filename))
                    cout << Color::GREEN << "  History exported to " << filename << Color::RESET << "\n";
                else
                    cout << Color::RED << "  [!] Could not open file for writing.\n" << Color::RESET;
                break;
            }
            case 8:
                running = false;
                cout << "\n" << Color::MAGENTA << "Thank you for using SettleUp. Goodbye!" << Color::RESET << "\n";
                break;
            default:
                cout << Color::RED << "  [!] Invalid choice, please try again.\n" << Color::RESET;
        }
    }

    return 0;
}
