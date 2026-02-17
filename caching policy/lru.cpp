struct Node{
    int key;
    int value;
    Node* next;
    Node* prev;
    Node(int k,int v){
        key = k;
        value = v;
        next = nullptr;
        prev = nullptr;
    }
};

class LRUCache {
private:
    std::size_t size;
    unordered_map<int, Node *> mp;
    Node* head;
    Node* tail;

    void remove(Node* node){
        Node* prev_node = node->prev;
        Node* next_node = node->next;
        next_node->prev = prev_node;
        prev_node->next = next_node; 
    }

    void add(Node* node){
        Node* next_node = head->next;
        head->next = node;
        node->prev = head;
        node->next = next_node;
        next_node->prev = node;
    }

public:
    LRUCache(int capacity) {
        size = capacity;
        head = new Node(-1,-1);
        tail = new Node(-1,-1);
        head->next = tail;
        tail->prev = head;
    }

    int get(int key) {
        if(mp.find(key)==mp.end()){
            return -1;
        }
        Node* node = mp[key];
        remove(node);
        add(node);
        return node->value;
    }
    
    void put(int key, int value) {
        if(mp.find(key)!=mp.end()){
            Node *old = mp[key];
            remove(old);
            delete(old);
        }
        Node* node = new Node(key,value);
        add(node);
        mp[key] = node;

        if(mp.size()>size){
            Node *del = tail->prev;
            remove(del);
            mp.erase(del->key);
            delete del;
        }
    }
};