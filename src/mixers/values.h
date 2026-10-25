/**
 * @file values.h
 * @brief Shared mixer values with one-key or two-key reservations.
 *
 * A mixer owns these lists; strips borrow entries. A send needs two keys
 * (channel, bus), while a bus mute needs one. Reserving the same keys returns
 * the same object, allowing backend replies to update several strips at once.
 * The wrappers reuse Interval's generic ValueList<T>: T selects the stored
 * record type, while these classes supply mixer-specific key matching.
 * Unlike StripMixerMixin, this use of templates shares storage management,
 * not UI behavior. OneIndexValueList/TwoIndexValueList contain that storage.
 *
 * @code{.cpp}
 * TwoIndexValueList sends;
 * TwoIndexValue* first = sends.reserve(5, 2);
 * TwoIndexValue* second = sends.reserve(5, 2); // same object
 * first->value = 0.5f;                       // second sees 0.5f too
 * sends.unreserve(first);
 * sends.unreserve(second);                  // last user: object is deleted
 * @endcode
 * Do not delete borrowed entries or keep using them after the final release.
 */

#ifndef _VALUES_h
#define _VALUES_h

#include <stdint.h>
#include "Interval.h"  // Generic stable-address reservation container

/*
values handled by mixers are shared between strips and interfaces.
They are pointers which allow immediate propagation to the GUI and other strips
when one value is modified.

Each stored object contains:
- a float value       : usually a level
- a bool toggle       : usually mute / on-off
- one or two indexes  : typically strip/channel and optional mix/bus
- a count             : number of users reserving the same object
*/


/*
Example with two indices

TwoIndexValueList sends;

TwoIndexValue* s1 = sends.reserve(5, 2);   // channel 5 -> mix 2
TwoIndexValue* s2 = sends.reserve(5, 2);   // same shared object

s1->toggle = true;

sends.unreserve(s1);
sends.unreserve(s2);                       // deleted here
*/


/**
 * @brief Shared mixer value, toggle and reservation count for one key.
 */
struct OneIndexValue {
    float value;        ///< usually level
    bool toggle;        ///< usually on/off or mute

    uint8_t i1;         ///< index 1 = usually channel #
    uint8_t count;      ///< reservation count

    /**
     * @brief Initialize OneIndexValue and its configured member bindings.
     */
    OneIndexValue()
        : value(0.0f), toggle(false), i1(0), count(0) {}
};

/**
 * @brief Shared mixer value identified by a pair of keys.
 */
struct TwoIndexValue : public OneIndexValue {
    uint8_t i2;         ///< index 2 = usually bus/mix #

    /**
     * @brief Initialize TwoIndexValue and its configured member bindings.
     */
    TwoIndexValue()
        : OneIndexValue(), i2(0) {}
};


/**
 * @brief Wrapper for values indexed by one index
 *
 * Public API kept close to the original version:
 * - reserve(i1)
 * - unreserve(ptr)
 * - get(i1)
 * - clear()
 * - size()
 */

class OneIndexValueList {
private:
    ValueList<OneIndexValue> list;

public:
    /** @brief Use the default initialization/destruction behavior. */
    OneIndexValueList() = default;
    /** @brief Release resources held by this object. */
    ~OneIndexValueList() = default;

    /** @brief Disable copying to preserve resource ownership. */
    OneIndexValueList(const OneIndexValueList&) = delete;
    /** @brief Disable assignment to prevent copying owned resources. */
    OneIndexValueList& operator=(const OneIndexValueList&) = delete;

    /**
     * @brief Reserve the shared value for one key, creating it only if absent.
     *
     * @param i1 Mixer key, normally a channel or bus id.
     * @return Stable borrowed pointer, or nullptr if allocation fails.
     * @note Pair with unreserve(); the final release deletes the value.
     */
    OneIndexValue* reserve(uint8_t i1) {
        return list.reserve(
            // matcher
            [i1](const OneIndexValue& v) {
                return (v.i1 == i1);
            },
            // initializer for a new object
            [i1](OneIndexValue& v) {
                v = OneIndexValue();
                v.i1 = i1;
            }
        );
    }

    /**
     * @brief unreserve(ptr)
     *
     * Decrements the reservation count.
     * Deletes the object if count becomes 0.
     */
    bool unreserve(OneIndexValue* ptr) {
        return list.unreserve(ptr);
    }

    /**
     * @brief get(i1)
     *
     * Returns the pointer to the object matching i1, or nullptr.
     */
    OneIndexValue* get(uint8_t i1) const {
        return list.get(
            [i1](const OneIndexValue& v) {
                return (v.i1 == i1);
            }
        );
    }

    /**
     * @brief Delete all shared values, invalidating every outstanding pointer.
     */
    void clear() {
        list.clear();
    }

    /** @brief Return the current number of entries. */
    uint16_t size() const {
        return list.size();
    }
};

/**
 * @brief Wrapper for values indexed by two indexes
 *
 * Public API kept close to the original version:
 * - reserve(i1, i2)
 * - unreserve(ptr)
 * - get(i1, i2)
 * - clear()
 * - size()
 */

class TwoIndexValueList {
private:
    ValueList<TwoIndexValue> list;

public:
    /** @brief Use the default initialization/destruction behavior. */
    TwoIndexValueList() = default;
    /** @brief Release resources held by this object. */
    ~TwoIndexValueList() = default;

    /** @brief Disable copying to preserve resource ownership. */
    TwoIndexValueList(const TwoIndexValueList&) = delete;
    /** @brief Disable assignment to prevent copying owned resources. */
    TwoIndexValueList& operator=(const TwoIndexValueList&) = delete;

    /**
     * @brief Reserve the shared value for a key pair, creating it only if absent.
     *
     * @param i1 First mixer key, normally an input channel.
     * @param i2 Second mixer key, normally a bus.
     * @return Stable borrowed pointer, or nullptr if allocation fails.
     * @note Pair with unreserve(); the final release deletes the value.
     */
    TwoIndexValue* reserve(uint8_t i1, uint8_t i2) {
        return list.reserve(
            // matcher
            [i1, i2](const TwoIndexValue& v) {
                return (v.i1 == i1) && (v.i2 == i2);
            },
            // initializer for a new object
            [i1, i2](TwoIndexValue& v) {
                v = TwoIndexValue();
                v.i1 = i1;
                v.i2 = i2;
            }
        );
    }

    /**
     * @brief unreserve(ptr)
     *
     * Decrements the reservation count.
     * Deletes the object if count becomes 0.
     */
    bool unreserve(TwoIndexValue* ptr) {
        return list.unreserve(ptr);
    }

    /**
     * @brief get(i1, i2)
     *
     * Returns the pointer to the object matching (i1, i2), or nullptr.
     */
    TwoIndexValue* get(uint8_t i1, uint8_t i2) {
        return list.get(
            [i1, i2](const TwoIndexValue& v) {
                return (v.i1 == i1) && (v.i2 == i2);
            }
        );
    }

    /**
     * @brief Delete all shared values, invalidating every outstanding pointer.
     */
    void clear() {
        list.clear();
    }

    /** @brief Return the current number of entries. */
    uint16_t size() const {
        return list.size();
    }
};


#endif



