#if !defined(CIRCULARLIST_H)
#define CIRCULARLIST_H

// 定义链表节点
struct Node {
    int data;
    Node* next;

    Node(int value) : data(value), next(nullptr) {}
};

// 定义循环链表
class CircularLinkedList {
private:
    Node* head;

public:
    CircularLinkedList() : head(nullptr) {}

    // 添加节点到链表
    void add(int value) {
        Node* newNode = new Node(value);
        if (head == nullptr) {
            head = newNode;
            head->next = head; // 指向自己形成循环
        } else {
            Node* temp = head;
            while (temp->next != head) { // 找到最后一个节点
                temp = temp->next;
            }
            temp->next = newNode; // 将新节点连接到最后一个节点
            newNode->next = head; // 新节点的next指向头节点
        }
    }

    // 打印链表
    void print() {
        if (head == nullptr) {
            std::cout << "Circular Linked List is empty." << std::endl;
            return;
        }
        Node* temp = head;
        do {
            std::cout << temp->data << " ";
            temp = temp->next;
        } while (temp != head);
        std::cout << std::endl;
    }

    // 销毁链表
    void destroy() {
        if (head == nullptr) return;

        Node* current = head;
        Node* next = nullptr;
        do {
            next = current->next;
            delete current;
            current = next;
        } while (current != head);

        head = nullptr;
    }

    // 析构函数
    ~CircularLinkedList() {
        destroy();
    }
};

#endif // CIRCULARLIST_H
