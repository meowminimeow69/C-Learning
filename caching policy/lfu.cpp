struct Node{
    int key;
    int value;
    int count;
    Node* prev;
    Node* next;
    Node(int k,int v){
        key = k;
        value = v;
        count = 1;
        prev =nullptr;
        next = nullptr;
    }
};

class LFUCache {
private:
  unordered_map<int,Node*> mp1;
  unordered_map<int,pair<Node*,Node*>> mp2;
  int min_freq;
  int size;

  void add(Node* node,int freq){
    if(mp2.find(freq)==mp2.end()){
        Node *head = new Node(-1, -1);
        Node *tail = new Node(-1, -1);
        head->next = tail;
        tail->prev = head;
        mp2[freq] = {head,tail};
    }

    Node *head = mp2[freq].first;
    Node *temp = head->next;
    node->next = temp;
    node->prev = head;
    head->next = node;
    temp->prev = node;
  }

  void remove(Node* node){
    Node* prev_node = node->prev;
    Node* next_node = node->next;
    prev_node->next = next_node;
    next_node->prev = prev_node;
  }

  void updatefreq(Node* node){
    int old_count = node->count;
    node->count++;
    remove(node);

    if(mp2[old_count].first->next ==mp2[old_count].second){
        mp2.erase(old_count);
        if(min_freq==old_count){
            min_freq++;
        }
    }
    add(node,node->count);
  }

public:
    LFUCache(int capacity) {
      size = capacity;
      min_freq = 0;
    }
    
    int get(int key) {
        if(mp1.find(key)==mp1.end()){
            return -1;
        }
        Node* node = mp1[key];
        int res = node->value;
        updatefreq(node);
        return res;
    }
    
    void put(int key, int value) {
        if(size==0){
            return;
        }
        // adds
        if(mp1.find(key)!=mp1.end()){
            Node* node = mp1[key];
            node->value = value;
            updatefreq(node);
        }
        else{
            if(mp1.size()==size){
                Node* node = mp2[min_freq].second->prev;
                mp1.erase(node->key);
                remove(node);

                if(mp2[min_freq].first->next == mp2[min_freq].second){
                    mp2.erase(min_freq);
                }
                delete node;
            }
            Node* new_node = new Node(key,value);
            mp1[key] = new_node;
            min_freq = 1;
            add(new_node,min_freq);
        }
    }
};