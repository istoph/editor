#ifndef DOCUMENTTEST_H
#define DOCUMENTTEST_H

class DocumentTestHelper {
public:
    Document &getDoc(File *f) {
        return f->_doc;
    }
};

#endif // DOCUMENTTEST_H
