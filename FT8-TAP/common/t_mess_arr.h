#ifndef _INCLUDE_T_MESS_ARR_H_
#define _INCLUDE_T_MESS_ARR_H_

typedef struct { uint8_t type; uint8_t to; char mess[400];} t_mess;

template<typename T>
class DynamicArray1 {
private:
	T* data_;
	size_t size_;
	size_t capacity_;

public:
	DynamicArray1() : data_(new T[2]), size_(0), capacity_(2) {}

	~DynamicArray1() {
		delete[] data_;
	}

	void resize(size_t new_capacity) {
		T* new_data = new T[new_capacity];
		if (new_capacity<=2) size_=0;
		if (new_capacity<size_) size_=0;
		for (size_t i = 0; i < new_capacity; i++) {
			new_data[i] = std::move(data_[i]);
		}
		delete[] data_;
		data_ = new_data;
		capacity_ = new_capacity;
	}

	void push_back(const T& value) {
		if (size_ == capacity_) {
			resize(capacity_+=10);
		}
		data_[size_++] = value;
	}

	void erase(size_t index) {
		if (index >= size_) {
			throw std::out_of_range("Invalid erasure index");
		}

		for (size_t i = index; i < size_ - 1; ++i) {
			data_[i] = std::move(data_[i+1]);
		}
		--size_;
	}

	T& operator[](size_t index) {
		if (index >= size_) {
			throw std::out_of_range("Index out of bounds");
		}
		return data_[index];
	}

	size_t size() const { return size_; }
	size_t capacity() const { return capacity_; }

	void DynArrayAdd( uint8_t mode, uint8_t to, char * add_mess) {
		t_mess sss;
		strcpy(sss.mess, add_mess);
		sss.type = mode;
		sss.to = to;
		push_back(sss);
	};
};

#endif // _INCLUDE_T_MESS_ARR_H_
