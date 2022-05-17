/**
 * @file HashMap.hpp
 * @author Aviad Dudkevich
 * @brief Implementation of HashMap class using generic keys and values.
 */
#include <iostream>
#include <vector>
#include <exception>

// Constants
const long TABLE_FACTOR = 2;
const int INITIAL_CAPACITY = 16;
const double DEFAULT_LOWER_LOAD_FACTOR = 0.25;
const double DEFAULT_UPPER_LOAD_FACTOR = 0.75;

static const char *MEMORY_ERROR_MSG = "Memory error occurred\n";
static const char *DIFFERENT_SIZE_VECTORS_ERROR_MSG = "HashMap constructor got vectors with"
                                                      " different sizes.";
static const char *INVALID_LOAD_FACTORS_LOWER_HIGHER = "HashMap must have lower load factor "
                                                       "smaller than upper load factor.\n";
static const char *INVALID_LOAD_FACTORS_OUT_RANGE = "HashMap lower and upper load factors"
                                                    " must to be between 0 and 1.\n";
static const char *KEY_DOSENT_EXIST_ERROR = "HashMap doesn't have an element with what "
                                            "key.\n";

using std::vector;
using std::pair;

/**
 * HashMap class - implementation of hash table database similar to STL interface.
 * @tparam KeyT type argument for generic key. Assumptions: have copy constructor, == operator
 * between two KeyT, support std::hash.
 * @tparam ValueT type argument for generic value. Assumptions: have copy constructor, default
 * constructor.
 */
template<typename KeyT, typename ValueT>
class HashMap
{
public:
    /**
     * Default constructor.
     */
    HashMap() : HashMap(DEFAULT_LOWER_LOAD_FACTOR, DEFAULT_UPPER_LOAD_FACTOR)
    {}

    /**
     * Constructor given lower and upper load factor.
     * @param lowerLoadFactor double.
     * @param upperLoadFactor double.
     */
    HashMap(const double &lowerLoadFactor, const double &upperLoadFactor) try :
            _capacity(INITIAL_CAPACITY), _size(0), _upperLoadFactor(upperLoadFactor),
            _lowerLoadFactor(lowerLoadFactor),
            _table(new vector<pair<KeyT, ValueT>>[_capacity])
    {
        if (_upperLoadFactor < _lowerLoadFactor)
        {
            throw std::invalid_argument(INVALID_LOAD_FACTORS_LOWER_HIGHER);
        }
        if (_lowerLoadFactor < 0 || _upperLoadFactor > 1)
        {
            throw std::invalid_argument(INVALID_LOAD_FACTORS_OUT_RANGE);
        }
    }
    catch (std::bad_alloc &ex)
    {
        std::cerr << MEMORY_ERROR_MSG;
        exit(EXIT_FAILURE);
    }

    /**
     * Constructor given a vector of keys and vector of values as initial input.
     * If the vectors have a different size, or vector key have two or more identical keys - throw
     * invalid_argument exception.
     * @param keyVector vector of KeyT.
     * @param valueVector vector of ValueT.
     */
    HashMap(const vector<KeyT> &keyVector, const vector<ValueT> &valueVector) : HashMap()
    {
        if (keyVector.size() != valueVector.size())
        {
            throw std::invalid_argument(DIFFERENT_SIZE_VECTORS_ERROR_MSG);
        }
        for (int i = 0; i < keyVector.size(); ++i)
        {
            if (containsKey(keyVector[i])) // if a same key appears again,
                // the new corresponding value should overwrite the old one.
            {
                vector<pair<KeyT, ValueT>> &bucket = _table[_getIndex(keyVector[i], _capacity)];
                for (auto j = bucket.begin(); j != bucket.end(); ++j)
                {
                    if (j->first == keyVector[i])
                    {
                        j->second = valueVector[i];
                    }
                }
            }
            else
            {
                _addToTable(keyVector[i], valueVector[i], _table, _capacity);
                ++_size;
            }
        }
        _keepUpperLoadFactor();
    }

    /**
     * Copy constructor.
     * @param other anther HashMap with the same KeyT and ValueT.
     */
    HashMap(const HashMap<KeyT, ValueT> &other) try :
            _capacity(other._capacity), _size(other._size),
            _upperLoadFactor(other._upperLoadFactor), _lowerLoadFactor(other._lowerLoadFactor),
            _table(new vector<pair<KeyT, ValueT>>[_capacity])
    {
        for (const pair<KeyT, ValueT> &p: other)
        {
            _addToTable(p.first, p.second, _table, _capacity);
        }
    }
    catch (std::bad_alloc &ex)
    {
        std::cerr << MEMORY_ERROR_MSG;
        exit(EXIT_FAILURE);
    }

    /**
     * Move constructor.
     * @param other rvalue reference to HashMap with the same KeyT and ValueT.
     */
    HashMap(HashMap<KeyT, ValueT> && other) noexcept : _capacity(other._capacity),
                                                      _size(other._size),
                                                      _upperLoadFactor(other._upperLoadFactor),
                                                      _lowerLoadFactor(other._lowerLoadFactor),
                                                      _table(other._table)
    {
        other._table = nullptr;
    }

    /**
     * Destructor.
     */
    ~HashMap()
    {
        delete[] _table;
    }

    /**
     * @return the number of elements in the HashMap.
     */
    inline int size() const
    { return (int) _size; }

    /**
     * @return HashMap capacity.
     */
    inline int capacity() const
    { return (int) _capacity; }

    /**
     * @return HashMap load factor in double.
     */
    inline double getLoadFactor() const
    { return (double) _size / _capacity; }

    /**
     * @return true if HashTable is empty, false otherwise.
     */
    inline bool empty() const
    { return _size == 0; }

    /**
     * Insert new value to the HashMap.
     * @param key KeyT value.
     * @param value ValueT value.
     * @return true if the value added successfully to the HashMap, false otherwise (in case
     * HashMap already have element with the same key).
     */
    bool insert(const KeyT &key, const ValueT &value)
    {
        if (containsKey(key))
        {
            return false;
        }
        else
        {
            _addToTable(key, value, _table, _capacity);
            ++_size;
            _keepUpperLoadFactor();
        }
        return true;
    }

    /**
     * Removing an element with the given key.
     * @param key KeyT value.
     * @return true if successful, false otherwise.
     */
    bool erase(const KeyT &key)
    {
        vector<pair<KeyT, ValueT>> &bucket = _table[_getIndex(key, _capacity)];
        for (auto i = bucket.begin(); i != bucket.end(); ++i)
        {
            if (i->first == key)
            {
                bucket.erase(i);
                --_size;
                _keepLowerLoadFactor();
                return true;
            }
        }
        return false;
    }

    /**
     * @param key KeyT value.
     * @return true if HashMap contain an element with this key.
     */
    bool containsKey(const KeyT &key) const
    {
        vector<pair<KeyT, ValueT>> &bucket = _table[_getIndex(key, _capacity)];
        if (bucket.size() > 0)
        {
            for (pair<KeyT, ValueT> &p: bucket)
            {
                if (p.first == key)
                {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @param key KeyT value.
     * @return const reference to the value of the element with the given key. Throw out_of_range
     * exception if HashMap doesn't contains the key.
     */
    const ValueT &at(const KeyT &key) const
    {
        vector<pair<KeyT, ValueT>> &bucket = _table[_getIndex(key, _capacity)];
        for (auto &i: bucket)
        {
            if (i.first == key)
            {
                return i.second;
            }
        }
        throw std::out_of_range(KEY_DOSENT_EXIST_ERROR);
    }

    /**
     * @param key KeyT value.
     * @return reference to the value of the element with the given key. Throw out_of_range
     * exception if HashMap doesn't contains the key.
     */
    ValueT &at(const KeyT &key)
    {
        // I was too lazy to find a way to avoid copying and pasting the same code. sorry.
        vector<pair<KeyT, ValueT>> &bucket = _table[_getIndex(key, _capacity)];
        for (auto &i: bucket)
        {
            if (i.first == key)
            {
                return i.second;
            }
        }
        throw std::out_of_range(KEY_DOSENT_EXIST_ERROR);
    }

    /**
     * @param key KeyT value.
     * @return reference to the value with the given key. If key is not in HashMap - insert key
     * and return a reference to the value.
     */
    ValueT &operator[](const KeyT &key)
    {
        vector<pair<KeyT, ValueT>> &bucket = _table[_getIndex(key, _capacity)];
        for (auto &i: bucket)
        {
            if (i.first == key)
            {
                return i.second;
            }
        }
        // key is not on the table.
        bucket.push_back(std::pair<KeyT, ValueT>(key, ValueT()));
        ++_size;
        _keepUpperLoadFactor();
        return operator[](key);
    }

    /**
     * @param key KeyT value.
     * @return the number of elements in the same bucket with that key. Throw out_of_range
     * exception if HashMap doesn't contains the key.
     */
    int bucketSize(const KeyT &key) const
    {
        if (containsKey(key))
        {
            return (int) _table[_getIndex(key, _capacity)].size();
        }
        else // I personally don't understand why to throw an exception if key is not on th HashMap.
            // In my opinion it should return the size of the bucket regardless if the bucket
            // contain the key.
        {
            throw std::out_of_range(KEY_DOSENT_EXIST_ERROR);
        }
    }

    /**
     * Erase all elements in HashMap.
     */
    void clear()
    {
        for (long i = 0; i < _capacity; ++i)
        {
            _table[i].clear();
        }
        _size = 0;
    }

    /**
     * assignment operator.
     * @param other anther HashMap with the same KeyT and ValueT.
     * @return a reference to this.
     */
    HashMap<KeyT, ValueT> &operator=(HashMap<KeyT, ValueT> other)
    {
        swap(*this, other);
        return *this;
    }

    /**
     * compare operator.
     * @param other anther HashMap with the same KeyT and ValueT.
     * @return true if the two HashMap have the same pairs of (KeyT, ValueT), false otherwise.
     */
    bool operator==(const HashMap<KeyT, ValueT> &other) const
    {
        if (_size == other._size &&
            _upperLoadFactor == other._upperLoadFactor &&
            _lowerLoadFactor == other._lowerLoadFactor &&
            _capacity == other._capacity) // personally I think that capacity is not
            // differentiate factor.
        {
            for (const pair<KeyT, ValueT> &p: other)
            {
                try
                {
                    if (at(p.first) != p.second)
                    {
                        return false;
                    }
                }
                catch (std::out_of_range &ex)
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    /**
     * compare operator.
     * @param other anther HashMap with the same KeyT and ValueT.
     * @return false if the two HashMap have the same pairs of (KeyT, ValueT), true otherwise.
     */
    inline bool operator!=(const HashMap<KeyT, ValueT> &other) const
    {
        return !(*this == other);
    }

    /**
     * HashMap Iterator class - pointer to pair.
     */
    class PointerToPair
    {
    public:
        /**
         * Constructor given a HashMap and flag for end.
         * @param hashMap The HashMap to point to its elements.
         * @param end true if point to the end of HashMap, true otherwise.
         */
        PointerToPair(const HashMap<KeyT, ValueT> &hashMap, const bool &end) : _hashMap(hashMap),
                                                                               _end(end),
                                                                               _bucketIndex(0),
                                                                               _vectorIterator()
        {
            _getNextBucketAndIterator();
            if (_end)
            {
                _vectorIterator = _hashMap._table[_hashMap._capacity - 1].end();
            }
        }

        /**
         * copy constructor.
         * @param other another PointerToPair.
         */
        PointerToPair(const PointerToPair &other) : _hashMap(other._hashMap), _end(other._end),
                                                    _bucketIndex(other._bucketIndex),
                                                    _vectorIterator(other._vectorIterator)
        {}

        /**
         * * operator.
         * @return dereference to const pair.
         */
        inline const pair<KeyT, ValueT> &operator*() const
        { return *_vectorIterator; }

        /**
         * -> operator.
         * @return const address to the pair.
         */
        inline const pair<KeyT, ValueT> *operator->() const
        { return &*_vectorIterator; }

        /**
         * prefix operator ++.
         * @return reference to this.
         */
        PointerToPair &operator++()
        {
            ++_vectorIterator;
            if (_vectorIterator == _hashMap._table[_bucketIndex].end())
            {
                ++_bucketIndex;
                _getNextBucketAndIterator();
            }
            return *this;
        }

        /**
         * suffix operator ++;
         * @return reference to PointerToPair of the previous pair.
         */
        PointerToPair operator++(int)
        {
            PointerToPair temp(*this);
            operator++();
            return temp;
        }

        /**
         * compare operator.
         * @param other another PointerToPair
         * @return true if point to the same pair in the same HashMap, false otherwise.
         */
        inline bool operator==(const PointerToPair &other) const
        {
            return &_hashMap == &other._hashMap &&
                   ((_end && other._end) || _vectorIterator == other._vectorIterator);
        }

        /**
         * compare operator.
         * @param other another PointerToPair
         * @return false if point to the same pair in the same HashMap, true otherwise.
         */
        inline bool operator!=(const PointerToPair &other) const
        { return !(*this == other); }

    private:
        const HashMap<KeyT, ValueT> &_hashMap; // the HashMap the PointerToPair belong to.
        bool _end; // flag to indicate the end of map.
        long _bucketIndex; // the current bucket the pointer at.
        typename vector<pair<KeyT, ValueT>>::iterator _vectorIterator; // Iterator of the bucket.

        /**
         * Get to the next bucket that is not empty and get the iterator of that bucket.
         */
        void _getNextBucketAndIterator()
        {
            while (_bucketIndex < _hashMap._capacity && _hashMap._table[_bucketIndex].empty())
            {
                ++_bucketIndex;
            }
            if (_bucketIndex != _hashMap._capacity)
            {
                _vectorIterator = _hashMap._table[_bucketIndex].begin();
            }
            else
            {
                _vectorIterator = _hashMap._table[_hashMap._capacity - 1].end();
                _end = true;
            }
        }
    };

    typedef PointerToPair iterator;
    typedef PointerToPair const_iterator;

    /**
     * Get iterator to the first pair in HashMap.
     * @return PointerToPair with pointer to the first pair.
     */
    const iterator begin() const
    { return iterator(*this, false); }

    /**
     * Get iterator to the first pair in HashMap.
     * @return PointerToPair with pointer to the first pair.
     */
    inline const const_iterator cbegin() const
    { return begin(); }

    /**
     * Get iterator to after the last pair.
     * @return PointerToPair with pointer to the next position after the last pair.
     */
    const iterator end() const
    { return iterator(*this, true); }

    /**
     * Get iterator to after the last pair.
     * @return PointerToPair with pointer to the next position after the last pair.
     */
    const const_iterator cend() const
    { return end(); }

    /**
     * Aid assignment operator.
     * @param first HashMap reference.
     * @param second HashMap reference.
     */
    friend void swap(HashMap<KeyT, ValueT> &first, HashMap<KeyT, ValueT> &second) noexcept
    {
        using std::swap;
        swap(first._capacity, second._capacity);
        swap(first._size, second._size);
        swap(first._upperLoadFactor, second._upperLoadFactor);
        swap(first._lowerLoadFactor, second._lowerLoadFactor);
        swap(first._table, second._table);
    }

private:
    long _capacity, _size; // capacity - how many buckets. size - how many pairs in HashMap.
    double _upperLoadFactor, _lowerLoadFactor; // to determine when to change table capacity.
    vector<pair<KeyT, ValueT>> *_table; // the table of the HashMap.

    /**
     * Get index in table by key value and table size.
     * @param key Key value.
     * @param tableSize long.
     * @return index in table.
     */
    inline static long _getIndex(const KeyT &key, const long &tableSize)
    { return std::hash<KeyT>{}(key) & (tableSize - 1); }

    /**
     * Add pair to the HashMap table.
     * @param key KeyT value.
     * @param value ValueT value.
     * @param table table to add the pair to.
     * @param tableSize the size of the table to add to.
     */
    inline static void _addToTable(const KeyT &key, const ValueT &value,
                                   vector<pair<KeyT, ValueT>> *&table, const long &tableSize)
    {
        table[_getIndex(key, tableSize)].push_back(pair<KeyT, ValueT>(key, value));
    }

    /**
     * ReHash to HashMap - insert all pairs to a new table.
     * @param newCapacity the capacity of the new table.
     */
    void _reHash(const long &newCapacity)
    {
        try
        {
            vector<pair<KeyT, ValueT>> *newTable = new vector<pair<KeyT, ValueT>>[newCapacity];
            for (iterator i = begin(); i != end(); ++i)
            {
                _addToTable(i->first, i->second, newTable, newCapacity);
            }
            delete[] _table;
            _table = newTable;
            _capacity = newCapacity;
        }
        catch (std::bad_alloc &ex)
        {
            std::cerr << MEMORY_ERROR_MSG;
            exit(EXIT_FAILURE);
        }
    }

    /**
     * check if load factor is not above _upperLoadFactor. If it does - change capacity and rehash.
     */
    void _keepUpperLoadFactor()
    {
        if (getLoadFactor() > _upperLoadFactor)
        {
            long newCapacity = _capacity;
            while (((double) _size / newCapacity) > _upperLoadFactor)
            {
                newCapacity *= TABLE_FACTOR;
            }
            _reHash(newCapacity);
        }
    }

    /**
     * check if load factor is not under _lowerLoadFactor. If it does - change capacity and rehash.
     */
    void _keepLowerLoadFactor()
    {
        if (getLoadFactor() < _lowerLoadFactor && _capacity != 1)
        {
            long newCapacity = _capacity / TABLE_FACTOR;
            _reHash(newCapacity);
        }
    }
};