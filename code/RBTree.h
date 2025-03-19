#ifndef RBTREE_H_
#define RBTREE_H_
#include <memory>
#include "MutexGuard.h"
#include <assert.h>
#include <stack>
#define RED true
#define BLACK false
#define LEFT 0
#define RIGHT 1

template<typename T>//这里T必须为非指针类型，思考如何改进。
class RBTree{
private:
    struct tree_node{
        T key;
        tree_node* children[2] = {nullptr, nullptr};
        tree_node* parent = nullptr;
        bool color = BLACK;
    };
    struct judge{
        bool left;
        bool right;
    };
    MutexLock mutex_;
    tree_node* root = nullptr;
    tree_node* nil = new tree_node;
public:

    bool insertNode(const T key){
        if(root == nullptr){
            root = new tree_node;
            root->color = BLACK;
            root->key = key;
            root->children[LEFT] = nil;
            root->children[RIGHT] = nil;
            return true;
        }
        MutexGuard lock(mutex_);
        tree_node* pnode = root;
        tree_node* parent_node = nullptr;
        while(pnode!=nil){
            parent_node = pnode;
            pnode = (key > pnode->key)?pnode->children[RIGHT]:pnode->children[LEFT];
        }
        tree_node* new_node = new tree_node;
        new_node->color = RED;
        new_node->key = key;
        new_node->children[LEFT] = nil;
        new_node->children[RIGHT] = nil;
        new_node->parent = parent_node;
        if(parent_node.key<key){
            parent_node->children[RIGHT] = new_node;
        }else{
            parent_node->children[LEFT] = new_node;
        }

        fixInsert(new_node);
    }
    bool fixInsert(tree_node* new_node){
        if(new_node->parent->color != RED){
            return true;
        }
        tree_node* uncle_node;
        auto grand_node = new_node->parent->parent;

        judge if_uncle_right;
        if_uncle_right.right = (new_node->parent==grand_node->children[LEFT])?RIGHT:LEFT;
        if_uncle_right.left = (if_uncle_right.right==RIGHT)?LEFT:RIGHT;
        uncle_node = new_node->parent->parent->children[if_uncle_right.right];

        if(uncle_node->color == RED){
            uncle_node->color = BLACK;
            new_node->parent->color = BLACK;
            new_node = new_node->parent->parent;
            if(new_node != root){
                new_node->color = RED;
                fixInsert(new_node);
            }else{
                return true;
            }
        }else{
            if(new_node->parent->children[if_uncle_right.left] == new_node){
                rotate(new_node->parent, if_uncle_right.right);
                grand_node->color = RED;
                new_node->parent->color = BLACK;
            }else{
                rotate(new_node->parent, if_uncle_right.left);
                rotate(grand_node, if_uncle_right.right);
                grand_node->color = RED;
                new_node->color = BLACK;
            }
        }
    }
    void rotate(tree_node* node, bool side){
        judge if_side_left;
        if_side_left.right = (side==LEFT)?RIGHT:LEFT;
        if_side_left.left = (side==LEFT)?LEFT:RIGHT;

        tree_node* child = node->children[if_side_left.right];
        bool if_right_child = (node->parent->children[RIGHT]==node)?RIGHT:LEFT;
        node->parent->children[if_right_child] = child;
        child->parent = node->parent;

        node->children[if_side_left.right] = child->children[if_side_left.left];
        node->children[if_side_left.right]->parent = node->children[if_side_left.right];

        child->children[if_side_left.left] = node;
        node->parent = child;
    }

    bool deleteNode(const T key){
        tree_node* node = getNodeByKey(key);

        if(!node){
            return false;
        }
        tree_node* poor_node;
        bool original_color = node->color;
        bool parent_right_right = (node->parent->children[RIGHT]==node)?RIGHT:LEFT;
        if(node->children[LEFT]==nil){
            node->children[RIGHT]->parent = node->parent;
            node->parent->children[parent_right_right] = node->children[RIGHT];
            poor_node = node->children[RIGHT];
        }else if(node->children[RIGHT]==nil){
            node->children[LEFT]->parent = node->parent;
            node->parent->children[parent_right_right] = node->children[LEFT];
            poor_node = node->children[LEFT];
        }else{
            tree_node* sub_node;
            sub_node = getMinimumRight(node);
            original_color = sub_node->color;
            assert(sub_node);
            poor_node = sub_node->children[RIGHT];
            if(sub_node!=node->children[RIGHT]){
                sub_node->children[RIGHT]->parent = sub_node->parent;
                sub_node->parent->children[LEFT] = sub_node->children[RIGHT];

                sub_node->children[RIGHT] = node->children[RIGHT];
                node->children[RIGHT]->parent = sub_node;
            }
            sub_node->parent = node->parent;
            node->parent->children[parent_right_right] = sub_node;

            sub_node->children[LEFT] = node->children[LEFT];
            node->children[LEFT]->parent = sub_node;
            sub_node->color = node->color;
        }
        delete node;
        
        if(original_color==BLACK){
            fixDelete(poor_node);
        }
    }
    bool fixDelete(tree_node* poor_node){
        while(poor_node->color==BLACK&&poor_node!=root){
            judge if_left_child;
            if_left_child.left = (poor_node->parent->children[LEFT]==poor_node)?LEFT:RIGHT;
            if_left_child.right = (if_left_child.left==LEFT)?RIGHT:LEFT;
            tree_node* bro_node = poor_node.parent[if_left_child.right];
            if(bro_node->color == RED){
                rotate(poor_node->parent, if_left_child.left);
                poor_node->parent->color = RED;
                bro_node->color = BLACK;
                continue;
            }else if(bro_node->color == BLACK){
                if(bro_node->children[if_left_child.right]->color==BLACK){
                    if(bro_node->children[if_left_child.left]->color==BLACK){
                        rotate(bro_node, if_left_child.right);
                        bro_node.color = RED;
                        poor_node = poor_node->parent;
                        continue;
                    }else{
                        rotate(bro_node, if_left_child.right);
                        bro_node->children[if_left_child.left]->color = BLACK;
                        bro_node->color = RED;
                        continue;
                    }
                }else{
                    rotate(poor_node->parent, if_left_child.left);
                    bro_node->color = poor_node->parent->color;
                    poor_node->parent->color = BLACK;
                    bro_node->children[if_left_child.right] = BLACK;
                    return true;
                }
            }
        }
        poor_node->color = RED;
    }
    tree_node* getNodeByKey(const T key){
        tree_node* pnode = root;
        while(pnode!=nil){
            if(key == pnode->key){
                return pnode;
            }
            pnode = (key > pnode->key)?pnode->children[RIGHT]:pnode->children[LEFT];
        }
        return nullptr;
    }
    tree_node* getMinimumRight(tree_node* node){
        if(node->children[RIGHT]==nil){
            return nullptr;
        }
        tree_node* tnode = node->children[RIGHT];

        while(tnode->children[LEFT]!=nil){
            tnode = tnode->children[LEFT];
        }
        return tnode;

    }
    void traverse(){
        std::stack<tree_node*> node_stack;
        node_stack.push(root);
        int i=0;
        while(!node_stack.empty()){
            auto node = node_stack.pop();
            print("node %d, color %d", i++, node->color);
            if(node->children[LEFT] != nil){
                node_stack.push(node->children[LEFT]);
            }
            if(node->children[RIGHT] != nil){
                node_stack.push(node->children[RIGHT]);
            }
        }
    }
    void clear(){
        std::stack<tree_node*> node_stack;
        node_stack.push(root);
        int i=0;
        while(!node_stack.empty()){
            auto node = node_stack.pop();
            if(node->children[LEFT] != nil){
                node_stack.push(node->children[LEFT]);
            }
            if(node->children[RIGHT] != nil){
                node_stack.push(node->children[RIGHT]);
            }
            delete node;
        }
        root = nullptr;
    }

};

#endif