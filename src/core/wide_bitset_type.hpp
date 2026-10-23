/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file wide_bitset_type.hpp Enum bit set for more values than fit in a single integer. */

#ifndef WIDE_BITSET_TYPE_HPP
#define WIDE_BITSET_TYPE_HPP

#include <bit>

/**
 * Enum-as-bit-set wrapper for more than 64 values.
 * Same interface as EnumBitSet, except that there is no single raw base() value;
 * use Word()/SetWord() to access the storage 64 bits at a time.
 * @tparam Tenum Enum values to wrap.
 * @tparam Tend_value Last valid value + 1.
 */
template <typename Tenum, Tenum Tend_value>
class WideEnumBitSet {
public:
	using ValueType = Tenum; ///< Value type of this bit set.
	static constexpr size_t BITS = to_underlying(Tend_value); ///< Number of valid values.
	static constexpr size_t WORDS = (BITS + 63) / 64; ///< Number of 64 bit storage words.

	constexpr WideEnumBitSet() : data{} {}

	/**
	 * Construct from a single enum value.
	 * @param value The single enum value to be set.
	 */
	constexpr WideEnumBitSet(Tenum value) : data{} { this->Set(value); }

	/**
	 * Construct from a list of enum values.
	 * @param values List of enum values.
	 */
	constexpr WideEnumBitSet(std::initializer_list<const Tenum> values) : data{}
	{
		for (const Tenum &value : values) this->Set(value);
	}

	/**
	 * Construct from a span of enum values. Invalid values are skipped.
	 * @param values Span of enum values.
	 */
	WideEnumBitSet(std::span<const Tenum> values) : data{}
	{
		for (const Tenum &value : values) {
			if (value < Tend_value) this->Set(value);
		}
	}

	/**
	 * Construct with the lowest 64 values taken from an integer.
	 * @param low Bits for values 0..63.
	 */
	explicit constexpr WideEnumBitSet(uint64_t low) : data{}
	{
		this->data[0] = low & WordMask(0);
	}

	constexpr auto operator <=>(const WideEnumBitSet &) const noexcept = default;

	/**
	 * Get a storage word.
	 * @param word Index of the word.
	 * @return Bits for values word*64 .. word*64+63.
	 */
	constexpr uint64_t Word(size_t word) const { return this->data[word]; }

	/**
	 * Set a storage word.
	 * @param word Index of the word.
	 * @param bits Bits for values word*64 .. word*64+63.
	 */
	constexpr void SetWord(size_t word, uint64_t bits) { this->data[word] = bits & WordMask(word); }

	constexpr WideEnumBitSet &Set()
	{
		for (size_t i = 0; i < WORDS; i++) this->data[i] = WordMask(i);
		return *this;
	}

	constexpr WideEnumBitSet &Set(Tenum value)
	{
		this->data[Index(value) / 64] |= Bit(value);
		return *this;
	}

	constexpr WideEnumBitSet &Set(const WideEnumBitSet &other)
	{
		for (size_t i = 0; i < WORDS; i++) this->data[i] |= other.data[i];
		return *this;
	}

	constexpr WideEnumBitSet &Set(Tenum value, bool set)
	{
		return set ? this->Set(value) : this->Reset(value);
	}

	constexpr WideEnumBitSet &Reset()
	{
		this->data = {};
		return *this;
	}

	constexpr WideEnumBitSet &Reset(Tenum value)
	{
		this->data[Index(value) / 64] &= ~Bit(value);
		return *this;
	}

	constexpr WideEnumBitSet &Reset(const WideEnumBitSet &other)
	{
		for (size_t i = 0; i < WORDS; i++) this->data[i] &= ~other.data[i];
		return *this;
	}

	constexpr WideEnumBitSet &Flip()
	{
		for (size_t i = 0; i < WORDS; i++) this->data[i] = ~this->data[i] & WordMask(i);
		return *this;
	}

	constexpr WideEnumBitSet &Flip(Tenum value)
	{
		this->data[Index(value) / 64] ^= Bit(value);
		return *this;
	}

	constexpr WideEnumBitSet &Flip(const WideEnumBitSet &other)
	{
		for (size_t i = 0; i < WORDS; i++) this->data[i] ^= other.data[i];
		return *this;
	}

	constexpr bool Test(Tenum value) const
	{
		return (this->data[Index(value) / 64] & Bit(value)) != 0;
	}

	constexpr bool All(const WideEnumBitSet &other) const
	{
		for (size_t i = 0; i < WORDS; i++) {
			if ((this->data[i] & other.data[i]) != other.data[i]) return false;
		}
		return true;
	}

	constexpr bool All() const
	{
		for (size_t i = 0; i < WORDS; i++) {
			if (this->data[i] != WordMask(i)) return false;
		}
		return true;
	}

	constexpr bool Any(const WideEnumBitSet &other) const
	{
		for (size_t i = 0; i < WORDS; i++) {
			if ((this->data[i] & other.data[i]) != 0) return true;
		}
		return false;
	}

	constexpr bool Any() const
	{
		for (size_t i = 0; i < WORDS; i++) {
			if (this->data[i] != 0) return true;
		}
		return false;
	}

	constexpr bool None() const { return !this->Any(); }

	constexpr WideEnumBitSet &operator|=(const WideEnumBitSet &other) { return this->Set(other); }

	constexpr WideEnumBitSet operator|(const WideEnumBitSet &other) const
	{
		WideEnumBitSet result = *this;
		return result.Set(other);
	}

	constexpr WideEnumBitSet &operator&=(const WideEnumBitSet &other)
	{
		for (size_t i = 0; i < WORDS; i++) this->data[i] &= other.data[i];
		return *this;
	}

	constexpr WideEnumBitSet operator&(const WideEnumBitSet &other) const
	{
		WideEnumBitSet result = *this;
		return result &= other;
	}

	/**
	 * Test that no bits outside the valid values are set.
	 * @returns true iff the bit set is valid.
	 */
	constexpr bool IsValid() const
	{
		for (size_t i = 0; i < WORDS; i++) {
			if ((this->data[i] & ~WordMask(i)) != 0) return false;
		}
		return true;
	}

	/**
	 * Count the number of set bits.
	 * @return The number of bits set to true.
	 */
	constexpr uint Count() const
	{
		uint count = 0;
		for (size_t i = 0; i < WORDS; i++) count += std::popcount(this->data[i]);
		return count;
	}

	/**
	 * Get the value of the Nth set bit.
	 * @param n The Nth set bit from which we want to know the value.
	 * @return The value of the Nth set bit, or std::nullopt if no Nth bit set.
	 */
	std::optional<Tenum> GetNthSetBit(uint n) const
	{
		for (auto i : *this) {
			if (n == 0) return i;
			--n;
		}
		return std::nullopt;
	}

	/** Forward iterator over the set values. */
	struct Iterator {
		using value_type = Tenum;
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::forward_iterator_tag;
		using pointer = void;
		using reference = Tenum;

		const WideEnumBitSet *set; ///< Bit set being iterated.
		size_t pos; ///< Current value, or BITS when done.

		constexpr Tenum operator*() const { return static_cast<Tenum>(this->pos); }
		constexpr bool operator==(const Iterator &other) const { return this->pos == other.pos; }

		constexpr Iterator &operator++()
		{
			this->pos = this->set->NextSet(this->pos + 1);
			return *this;
		}

		constexpr Iterator operator++(int)
		{
			Iterator result = *this;
			++*this;
			return result;
		}
	};

	constexpr Iterator begin() const { return Iterator{this, this->NextSet(0)}; }
	constexpr Iterator end() const { return Iterator{this, BITS}; }

private:
	std::array<uint64_t, WORDS> data; ///< Bitmask of values.

	static constexpr size_t Index(Tenum value) { return to_underlying(value); }
	static constexpr uint64_t Bit(Tenum value) { return 1ULL << (Index(value) % 64); }

	static constexpr uint64_t WordMask(size_t word)
	{
		size_t bits = std::min<size_t>(64, BITS - word * 64);
		return bits == 64 ? UINT64_MAX : (1ULL << bits) - 1;
	}

	/**
	 * Find the first set value at or after a position.
	 * @param pos Position to start searching at.
	 * @return The found value, or BITS if there is none.
	 */
	constexpr size_t NextSet(size_t pos) const
	{
		while (pos < BITS) {
			uint64_t word = this->data[pos / 64] >> (pos % 64);
			if (word != 0) return pos + std::countr_zero(word);
			pos = (pos / 64 + 1) * 64;
		}
		return BITS;
	}
};

#endif /* WIDE_BITSET_TYPE_HPP */
