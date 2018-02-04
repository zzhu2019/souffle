// unordered_multiset::begin/end example
#include <iostream>
#include <string>
#include <unordered_set>
#include <array>

typedef uint32_t Element;
typedef std::array<uint32_t, 4> Tuple;

/**
 * Define a templatized equal function for tuples 
 */

template <unsigned... List>
struct TupleEqual;

/** Base case: compare single element */
template<unsigned Idx>
struct TupleEqual<Idx> {
    inline bool operator()(const Tuple& a, const Tuple &b) const {
        static_assert(Idx < std::tuple_size<Tuple>::value, "index out of bounds!");
        return a[Idx] == b[Idx];
    }
};

/** Recursive case: compare recursively a list of keys */
template <unsigned Idx, unsigned... Rest>
struct TupleEqual<Idx, Rest...> {
    inline bool operator()(const Tuple& a, const Tuple &b) const {
        return TupleEqual<Idx>{}(a,b) &&
               TupleEqual<Rest...>{}(a,b);
    }
};

/**
 * Define a templatized hash function for tuples 
 */

template <unsigned... List>
struct TupleHash;

/** Base case for a single element: use standard template hash function for tuple elements */ 
template<unsigned Idx>
struct TupleHash<Idx> {
    inline std::size_t operator()(const Tuple& t) const {
        static_assert(Idx < std::tuple_size<Tuple>::value, "index out of bounds!");
        return std::hash<Element>{}(t[Idx]);
    }
};

/** Recursive case: combine hash values of elements */
template <unsigned Idx, unsigned... Rest>
struct TupleHash<Idx, Rest...> {
    inline std::size_t operator()(const Tuple& t) const {
        std::size_t h = TupleHash<Rest...>{}(t);
        return TupleHash<Idx>{}(t) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
};

/** Generic hash indexed table */
template <typename Tuple, unsigned... Index>
struct HashedIndexRelation {
    /** Define multiset for hashed index relation */
    typedef std::unordered_multiset<Tuple, TupleHash<Index...>, TupleEqual<Index...> > data_structure;
    typedef typename data_structure::key_type key_type;

    typedef typename data_structure::iterator iterator;

 
private:
    data_structure index; 

public:
    /** Is empty */
    bool empty() const {
        return index.empty();
    }

    /** Size of index */
    std::size_t size() const {
        return index.size();

    }

    /** Iterator to begin */
    inline iterator begin() {
        return index.begin();
    }

    /** Iterator to end */ 
    inline iterator end() {
        return index.end();
    }

    /* Does a tuple exists? */
    bool contains(const key_type& key) const {
       // check for existence of a tuple 
       std::pair<iterator,iterator> range = index.equal_range(key);
       for (iterator it=range.first; it!=range.second; ++it) {
           // data-structure has only notion of partial existence
           if (*it == key) {
               return true;
           }
       }
       return false;
    }

    /** Insert a tuple: return false if tuple already exists; false otherwise */
    bool insert(const key_type &key) {
       // only insert if it does not exists
       if (!contains(key)) { 
           index.insert(key);
       }
       return true;
    } 

    /** Get equal range */
    std::pair<iterator, iterator> equalRange(const key_type& key) {
       return index.equal_range(key);  
    }
};

/** Print tuple */ 

int main()
{

    Tuple x1 = {1,2,3,4};
    Tuple x2 = {1,2,4,5};
    Tuple x3 = {2,1,4,5};
    Tuple x4 = {3,2,4,5};



    // same key because only first and second element of tuple
    // is used. 
    std::size_t h1 = TupleHash<0,1>{}(x1); 
    std::size_t h2 = TupleHash<0,1>{}(x2); 
    std::cout << h1 << "\t";
    std::cout << h2 << ((h1 == h2)?" OK":" FAILED") << std::endl;

    HashedIndexRelation<Tuple,0,1> r1;
    r1.insert(x1);
    r1.insert(x2);
    r1.insert(x3);
    r1.insert(x4);
    r1.insert(x4);

    // print relation
    std::cout << "Print relation\n";
    for(const Tuple &t : r1) { 
       for(int i=0;i<4;i++) {
           std::cout << t[i] << "\t";
        }
        std::cout << std::endl;
    }

    // print equal range 
    std::cout << "Print equal range for <1,2,_,_>\n";
    typedef HashedIndexRelation<Tuple,0,1>::iterator It;
    Tuple key={1,2,4,5}; // only first two elements count
    auto range = r1.equalRange(key);
    for (It f=range.first; f!=range.second; ++f) {
       for(int i=0;i<4;i++) {
           std::cout << (*f)[i] << "\t";
        }
        std::cout << std::endl;
    }

    return 0;
}
