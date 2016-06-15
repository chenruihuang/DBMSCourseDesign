#ifndef __L_ITEM_H
#define __L_ITEM_H

class LItem {
    public:
        LItem(int index = -1, float value = -1.0f): index_(index), value_(value) {}
		void set(int index, float value);
		int getIndex() const;
		float getValue() const;
    private:
        int index_;
        float value_;
};
#endif
