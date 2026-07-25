# SettleUp Expense Manager

This project was developed as part of my Data Structures coursework using C++. It helps simplify expense settlement among a group of people by calculating the minimum number of transactions needed to clear all pending payments.

Instead of everyone paying everyone else separately, the program calculates each person's final balance and generates a much simpler settlement plan.

---

## Features

- Add people to the expense group.
- Record shared expenses.
- Record direct debts between two people.
- View each person's net balance.
- Calculate the minimum number of transactions required.
- View transaction history with timestamps.
- Export the transaction history to a text file.
- Simple menu-driven console interface.
- Colored terminal output for better readability.

---

## Data Structures Used

### Graph (Adjacency Matrix)

- Used to store all debts between people.
- Each entry in the matrix represents how much one person owes another.
- Makes it easy to calculate everyone's final balance.

### Priority Queue (Max Heap)

- Used to store creditors and debtors separately.
- Always selects the person with the highest pending amount first.
- Helps generate the settlement plan efficiently.

### Multiset

- Stores the final settlement transactions.
- Automatically keeps them sorted before displaying them.

### Unordered Map

- Maps each person's name to a unique index.
- Allows quick lookup while updating or accessing records.

---

## How the Program Works

1. Add all the people involved in the expense group.

2. Record shared expenses or direct debts using the menu.

3. Every transaction is stored in an adjacency matrix.

4. The program calculates the net balance of each person.
   - Positive balance → Person should receive money.
   - Negative balance → Person needs to pay money.

5. Two Priority Queues are created:
   - One for creditors.
   - One for debtors.

6. The person who owes the most is matched with the person who should receive the most.

7. The payment amount is calculated, and any remaining balance is added back to the queue.

8. The process continues until every person's balance becomes zero.

9. Finally, the minimum settlement transactions are displayed.

---

## Project Structure

```
SettleUp-Expense-Manager/
│── main.cpp
│── README.md
│── LICENSE
└── .gitignore
```

---

## Compile

```bash
g++ main.cpp -o SettleUp
```

---

## Run

```bash
.\SettleUp.exe
```

---

## Future Improvements

Some features that can be added in future versions are:

- Split expenses by custom percentages instead of equal shares.
- Save and load expense data from files.
- Edit or delete previously added expenses.
- Export settlement reports in PDF or CSV format.
- Add a graphical user interface (GUI).

---

## Author

**Vasav Kumar**  
B.Tech CSE Student

GitHub: https://github.com/kvasav512
