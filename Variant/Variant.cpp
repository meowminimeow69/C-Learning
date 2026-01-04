// Variant.cpp 
#include <iostream>
#include<variant>
#include <string>

template<typename... Args>
struct get_first_type;

template<typename Head,typename...Tail>
struct get_first_type<Head, Tail...> {
    using type = Head;
};
// the above code is to get the head type 


template<typename T,typename... Types>
struct get_index_type;

template<typename T,typename... Types>
struct get_index_type<T, T, Types...> {
    static constexpr size_t value = 0;
};

template<typename T,typename Head,typename... Tail>
struct get_index_type<T, Head, Tail...> {
    static constexpr size_t value = 1 + get_index_type<T, Tail...>::value;
};
// the above code is to get the index for a give type 

//parent varaint storage , we will use this for recursion to go to base
template<typename Type, typename... Tails>
union variant_storage {
    Type value;
    variant_storage<Tails...> rest;

    // base recursion to set value
    template<typename other>
    variant_storage(std::integral_constant<size_t, 0>, other&& value)
    : value(std::forward<other>(value)) {}

    //we fill up the values using recursion and go to the index needed , leaves the current one and goes to next one to modify
    template<std::size_t Index,typename other>
    variant_storage(std::integral_constant<size_t, Index>, other&& value)
    : rest(std::integral_constant<size_t, Index - 1>{},std::forward<other>(value)) {}

    template<size_t Index>
    auto& get() {
        if constexpr (Index == 0) return value;
        else return rest. template get<Index - 1>(); // recursion here to get the value
    }

    void destroy(size_t Index) {
        if (Index == 0) value.~Type();
        else rest.destroy(Index - 1);
    }

    ~variant_storage(){}
};
// variant storage stores in a union , which has itself and tail 


template<typename Type>
union variant_storage<Type> {
    Type value;

    template<typename other>
    variant_storage(std::integral_constant<size_t, 0>, other&& value)
    : value(std::forward<other>(value) ){} // the process ends here for index wise recursion so I need to define in such a manner than I do recursion

    template<size_t Index>
    auto& get() {return value;}

    void destroy(size_t index) {value.~Type();}
};
//default varaint storage, I will build this first and then build the others  


template<typename... Args>
class Variant {
private:
    size_t index; // active index
    variant_storage<Args...> storage;
public:
    using first_type = typename get_first_type<Args...>::type; // write the meta programming code to get first type

    Variant(const Variant& other) = default;
    Variant(Variant&& other) = default;
    Variant& operator=(const Variant& other)= default;
    Variant& operator=(Variant&& other) = default;

    // default initalization 
    Variant()
    :index(0),
    storage(std::integral_constant<size_t,0>{},first_type{}) {}
    
    // specalized intialization
    template<typename Type>
    Variant(Type&& value)
    :index(get_index_type<Type,Args...>::value) ,
    storage(std::integral_constant<size_t,get_index_type<Type,Args...>::value>{},std::forward<Type>(value)) {}

    ~Variant() {
        storage.destroy(index);
    }

    template<size_t Index>
    auto& get() {
        return storage. template get<Index>();
    }
    //assignment operation using this , we will destroy the get the index first and then destroy the data, and assigned new data to storage and assigned new index
    template<typename Type>
    auto& operator=(Type&& value) {
        constexpr size_t type_idx = get_index_type<Type, Args...>::value;
        storage.destroy(index);
        
        new (&storage) variant_storage<Args...>(
            std::integral_constant<size_t, type_idx>{},
            std::forward<Type>(value)
        );

        index = type_idx;
        return *this;
    }

};


int main()
{
    std::cout << "Question on std::Variant Implementation"<<std::endl;
    /*
    Introduce in this manner, firstly I need to intalize a long possibility of input types to template metaprogramming is needed

    I also want the entire process in complie time , I will use recursion to navigate everywhere

    so I know that the base class has index and variant_storage data types in them.
    Next to store it for the first time, I will need to default initalize the first index, so I need to write function in meta programming to get the first index and first type

    for the value based intialization , I need to first find the index value and then modify that based on the index 
    
    so now to once I get the index, I will focus on the variant_storage desgin,
    since I am using recursion to desgin , so all the access of this is at index 0, for access 

    so I have a parent <T,Args...> annd <T> where I store the values , and then in the parents, to access each value, I will do it using recursion and go till index gets to zero to modify in the child
    
    */
    Variant <int, std::string, size_t> var;
    var = std::string("Hello");
    std::cout << "The value of variant : " << var.get<1>() << std::endl;
    var = size_t(1000);
    std::cout << "The value of variant : " << var.get<2>() << std::endl;
    var = int(2000);
    std::cout << "The value of variant : " << var.get<0>() << std::endl;


}

