
namespace souffle {

class ExplainDisplay {
private:
    SouffleProgram& prog;

    bool ncurses;
    WINDOW* treePad;
    WINDOW* queryWindow;
    int maxx, maxy;

    int depthLimit;

    // parse relation, split into relation name and values
    std::pair<std::string, std::vector<std::string>> parseRel(std::string rel) {
        // remove spaces
        rel.erase(std::remove(rel.begin(), rel.end(), ' '), rel.end());

        // remove last closing parenthesis
        if (rel.back() == ')') {
            rel.pop_back();
        }

        auto splitRel = split(rel, '(');
        std::string relName;
        if (splitRel.size() > 0) {
            relName = splitRel[0];
        }
        std::vector<std::string> args;

        if (splitRel.size() > 1) {
            args = split(splitRel[1], ',');
        }

        return std::make_pair(relName, args);
    }

    // initialise ncurses window
    WINDOW* makeQueryWindow() {
        WINDOW* w = newwin(3, maxx, maxy - 2, 0);
        wrefresh(w);
        return w;
    }

    // print provenance tree
    void printTree(std::unique_ptr<TreeNode> t) {
        if (t) {
            t->place(0, 0);
            ScreenBuffer* s = new ScreenBuffer(t->getWidth(), t->getHeight());
            t->render(*s);
            if (ncurses) {
                wprintw(treePad, s->getString().c_str());
            } else {
                std::cout << s->getString();
            }
        }
    }

    // print string
    void printStr(std::string s) {
        if (ncurses) {
            wprintw(treePad, s.c_str());
        } else {
            std::cout << s;
        }
    }

    // allow scrolling of provenance tree
    void scrollTree(int maxx, int maxy) {
        int x = 0;
        int y = 0;

        while (1) {
            int ch = wgetch(treePad);

            if (ch == KEY_LEFT) {
                if (x > 2) x -= 3;
            } else if (ch == KEY_RIGHT) {
                if (x < MAX_TREE_WIDTH - 3) x += 3;
            } else if (ch == KEY_UP) {
                if (y > 0) y -= 1;
            } else if (ch == KEY_DOWN) {
                if (y < MAX_TREE_HEIGHT - 1) y += 1;
            } else {
                break;
            }

            prefresh(treePad, y, x, 0, 0, maxy - 3, maxx - 1);
        }
    }

    // initialise ncurses window
    void initialiseWindow() {
        initscr();

        // get size of window
        getmaxyx(stdscr, maxy, maxx);

        // create windows for query and tree display
        queryWindow = makeQueryWindow();
        treePad = newpad(MAX_TREE_HEIGHT, MAX_TREE_WIDTH);

        keypad(treePad, true);
    }

public:
    ProvenanceDisplay(SouffleProgram& p, bool ncurses, int d = 4)
            : prog(p), ncurses(ncurses), depthLimit(d) {}

    void explain() {
        if (ncurses) {
            initialiseWindow();
            std::signal(SIGWINCH, NULL);
        }

        // process commands
        char buf[100];
        std::string line;

        ProvenanceTree provTree(prog, depthLimit);

        while (1) {
            if (ncurses) {
                // reset command line on each loop
                werase(queryWindow);
                wrefresh(queryWindow);
                mvwprintw(queryWindow, 1, 0, "Enter command > ");
                curs_set(1);
                echo();

                // get next command
                wgetnstr(queryWindow, buf, 100);
                noecho();
                curs_set(0);
                line = buf;

                // reset tree display on each loop
                werase(treePad);
                prefresh(treePad, 0, 0, 0, 0, maxy - 3, maxx - 1);
            } else {
                std::cout << "Enter command > ";
                getline(std::cin, line);
            }

            std::vector<std::string> command = split(line, ' ', 1);

            if (command[0] == "setdepth") {
                try {
                    depthLimit = atoi(command[1].c_str());
                } catch (std::exception& e) {
                    printStr("Usage: setdepth <depth>\n");
                    continue;
                }
                printStr("Depth is now " + std::to_string(depthLimit) + "\n");
            } else if (command[0] == "explain") {
                std::pair<std::string, std::vector<std::string>> query;
                if (command.size() > 1) {
                    query = parseRel(command[1]);
                } else {
                    printStr("Usage: explain relation_name(<element1>, <element2>, ...)\n");
                    continue;
                }
                elements tuple_elements(query.second);
                std::unique_ptr<TreeNode> t = provTree.getTree(query.first, tuple_elements);
                printTree(std::move(t));
            } else if (command[0] == "subproof") {
                std::pair<std::string, std::vector<std::string>> query;
                int label = -1;
                if (command.size() > 1) {
                    query = parseRel(command[1]);
                    label = atoi(query.second[0].c_str());
                } else {
                    printStr("Usage: subproof relation_name(<label>)\n");
                    continue;
                }
                std::unique_ptr<TreeNode> t = provTree.getTree(query.first, label);
                printTree(std::move(t));
            } else if (command[0] == "rule") {
                try {
                    auto query = split(command[1], ' ');
                    printStr(provTree.getRule(query[0], atoi(query[1].c_str())));
                } catch (std::exception& e) {
                    printStr("Usage: rule <rule number>\n");
                    continue;
                }
            } else if (command[0] == "exit") {
                break;
            }

            // refresh treePad and allow scrolling
            if (ncurses) {
                prefresh(treePad, 0, 0, 0, 0, maxy - 3, maxx - 1);
                scrollTree(maxx, maxy);
            }
        }
        if (ncurses) {
            endwin();
        }
    }
};

inline void explain(SouffleProgram& prog, bool ncurses = true) {
    std::cout << "Explain is invoked.\n";

    ExplainDisplay prov(prog, ncurses);
    prov.explain();
}

} // end of namespace souffle
