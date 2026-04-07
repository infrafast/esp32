#ifndef _VALUES_h
#define _VALUES_h

#include <stdint.h>

/* 
values handled by mixers are shared between stirps and mixers
they are pointeurs which allow immediate propagation to the GUI and other strips when one value is modified
hence it is composed of a values which is located at indexes corresponding to the dropdown of strips (which are channels and buses ID)
*/
struct OneIndexValue {
    float value;
    uint8_t i1;     // index 1 = usually channel #
    uint8_t count;

    OneIndexValue() : value(0.0f), i1(0), count(0) {}
};

struct TwoIndexValue : public OneIndexValue {
    uint8_t i2;     // index 2 = usually bus/mix #

    TwoIndexValue() : OneIndexValue(), i2(0) {}
};

/*
Voici une version embarquée avec liste chaînée simple et pointeurs stables.

L’idée :

chaque set(...) cherche si l’objet existe déjà
s’il existe :
met à jour value
incrémente count
renvoie un pointeur stable vers l’objet
sinon :
alloue un nouveau nœud
l’ajoute à la liste
initialise count = 1
unset(...) décrémente count puis supprime le nœud si count == 0
get(...) renvoie un pointeur vers l’objet, ou nullptr

Comme les objets sont stockés dans des nœuds alloués individuellement, les pointeurs restent stables tant que l’objet n’est pas supprimé.
*/

class OneIndexValueList {              // used when you only have one value
private:
    struct Node {
        OneIndexValue data;
        Node* next;

        Node(uint8_t index1, float v) : next(nullptr) {
            data.i1 = index1;
            data.value = v;
            data.count = 1;
        }
    };

    Node* head;

public:
    OneIndexValueList() : head(nullptr) {}

    ~OneIndexValueList() {
        clear();
    }

    OneIndexValue* set(uint8_t i1, float value) {
        Node* current = head;

        while (current != nullptr) {
            if (current->data.i1 == i1) {
                current->data.value = value;
                current->data.count++;
                return &current->data;
            }
            current = current->next;
        }

        Node* newNode = new Node(i1, value);
        if (newNode == nullptr) {
            return nullptr;
        }

        newNode->next = head;
        head = newNode;
        return &newNode->data;
    }

    bool unset(uint8_t i1) {
        Node* current = head;
        Node* previous = nullptr;

        while (current != nullptr) {
            if (current->data.i1 == i1) {
                if (current->data.count > 0) {
                    current->data.count--;
                }

                if (current->data.count == 0) {
                    if (previous == nullptr) {
                        head = current->next;
                    } else {
                        previous->next = current->next;
                    }
                    delete current;
                }
                return true;
            }

            previous = current;
            current = current->next;
        }

        return false;
    }

    OneIndexValue* get(uint8_t i1) {
        Node* current = head;

        while (current != nullptr) {
            if (current->data.i1 == i1) {
                return &current->data;
            }
            current = current->next;
        }

        return nullptr;
    }

    const OneIndexValue* get(uint8_t i1) const {
        Node* current = head;

        while (current != nullptr) {
            if (current->data.i1 == i1) {
                return &current->data;
            }
            current = current->next;
        }

        return nullptr;
    }

    void clear() {
        Node* current = head;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        head = nullptr;
    }

    uint16_t size() const {
        uint16_t n = 0;
        Node* current = head;
        while (current != nullptr) {
            n++;
            current = current->next;
        }
        return n;
    }
};


class TwoIndexValueList {
private:
    struct Node {
        TwoIndexValue data;
        Node* next;

        Node(uint8_t index1, uint8_t index2, float v) : next(nullptr) {
            data.i1 = index1;
            data.i2 = index2;
            data.value = v;
            data.count = 1;
        }
    };

    Node* head;

public:
    TwoIndexValueList() : head(nullptr) {}

    ~TwoIndexValueList() {
        clear();
    }

    TwoIndexValue* set(uint8_t i1, uint8_t i2, float value) {
        Node* current = head;

        while (current != nullptr) {
            if ((current->data.i1 == i1) && (current->data.i2 == i2)) {
                current->data.value = value;
                current->data.count++;
                return &current->data;
            }
            current = current->next;
        }

        Node* newNode = new Node(i1, i2, value);
        if (newNode == nullptr) {
            return nullptr;
        }

        newNode->next = head;
        head = newNode;
        return &newNode->data;
    }

    bool unset(uint8_t i1, uint8_t i2) {
        Node* current = head;
        Node* previous = nullptr;

        while (current != nullptr) {
            if ((current->data.i1 == i1) && (current->data.i2 == i2)) {
                if (current->data.count > 0) {
                    current->data.count--;
                }

                if (current->data.count == 0) {
                    if (previous == nullptr) {
                        head = current->next;
                    } else {
                        previous->next = current->next;
                    }
                    delete current;
                }
                return true;
            }

            previous = current;
            current = current->next;
        }

        return false;
    }

    TwoIndexValue* get(uint8_t i1, uint8_t i2) {
        Node* current = head;

        while (current != nullptr) {
            if ((current->data.i1 == i1) && (current->data.i2 == i2)) {
                return &current->data;
            }
            current = current->next;
        }

        return nullptr;
    }

    const TwoIndexValue* get(uint8_t i1, uint8_t i2) const {
        Node* current = head;

        while (current != nullptr) {
            if ((current->data.i1 == i1) && (current->data.i2 == i2)) {
                return &current->data;
            }
            current = current->next;
        }

        return nullptr;
    }

    void clear() {
        Node* current = head;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        head = nullptr;
    }

    uint16_t size() const {
        uint16_t n = 0;
        Node* current = head;
        while (current != nullptr) {
            n++;
            current = current->next;
        }
        return n;
    }
};


#endif



