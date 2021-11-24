
#ifndef SQL_EXPRESSIONS
#define SQL_EXPRESSIONS

#include "MyDB_AttType.h"
#include "MyDB_Table.h"
#include "MyDB_Catalog.h"
#include <string>
#include <vector>
#include <functional>
#include <map>

// create a smart pointer for database tables
using namespace std;
class ExprTree;
typedef shared_ptr <ExprTree> ExprTreePtr;

// this class encapsules a parsed SQL expression (such as "this.that > 34.5 AND 4 = 5")

// class ExprTree is a pure virtual class... the various classes that implement it are below
class ExprTree {

public:
	virtual string toString () = 0;
	virtual ~ExprTree () {}
	virtual MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) = 0;
	virtual vector<std::pair<string, string>> getIdentifiers() = 0;
};

class BoolLiteral : public ExprTree {

private:
	bool myVal;
public:
	
	BoolLiteral (bool fromMe) {
		myVal = fromMe;
	}

	string toString () {
		if (myVal) {
			return "bool[true]";
		} else {
			return "bool[false]";
		}
	}	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		return make_shared <MyDB_BoolAttType>();
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		return vector<std::pair<string, string>>();
	}
	
	~BoolLiteral () {}
	
};

class DoubleLiteral : public ExprTree {

private:
	double myVal;
public:

	DoubleLiteral (double fromMe) {
		myVal = fromMe;
	}

	string toString () {
		return "double[" + to_string (myVal) + "]";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		return make_shared <MyDB_DoubleAttType>();
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		return vector<std::pair<string, string>>();
	}
	
//	string getType (map <string, string> tableDef, map <string, map <string, string>> allInfo) {
//		return "double";
//	}
//	
//	string toString () {
//		return "double[" + to_string (myVal) + "]";
//	}	
	
	~DoubleLiteral () {}
};

// this implement class ExprTree
class IntLiteral : public ExprTree {

private:
	int myVal;
public:

	IntLiteral (int fromMe) {
		myVal = fromMe;
	}

	string toString () {
		return "int[" + to_string (myVal) + "]";
	}
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		return make_shared <MyDB_IntAttType> ();
	}
	
	
	vector<std::pair<string, string>> getIdentifiers() override {
		return vector<std::pair<string, string>>();
	}

	~IntLiteral () {}
};

class StringLiteral : public ExprTree {

private:
	string myVal;
public:

	StringLiteral (char *fromMe) {
		fromMe[strlen (fromMe) - 1] = 0;
		myVal = string (fromMe + 1);
	}

	string toString () {
		return "string[" + myVal + "]";
	}
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		return make_shared <MyDB_StringAttType> ();
	}
	
	
	vector<std::pair<string, string>> getIdentifiers() override {
		return vector<std::pair<string, string>>();
	}

	~StringLiteral () {}
};

class Identifier : public ExprTree {

private:
	string tableName;
	string attName;
public:

	Identifier (char *tableNameIn, char *attNameIn) {
		tableName = string (tableNameIn);
		attName = string (attNameIn);
	}

	string toString () {
		return "[" + tableName + "_" + attName + "]";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		
		string tableFullName = "";
		for (auto& namePair: tableNameGetter) {
			if (namePair.second == tableName) {
				tableFullName = namePair.first;
			}
		}
		
		MyDB_Table table;
		if (!table.fromCatalog(tableFullName, catalog)) {
			std::cout << "table does not exist" << std::endl;
			return nullptr;
		}
		auto attType = table.getSchema()->getAttByName(attName).second;
		if (attType == nullptr) {
			std::cout << "attribute does not exist in the table" << std::endl;
		}
		return attType;
	} 
	
	
	vector<std::pair<string, string>> getIdentifiers() override {
		vector<std::pair<string, string>> ret;
		ret.push_back(std::make_pair(tableName, attName));
		return ret;
	}

	~Identifier () {}
};

class MinusOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	MinusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "- (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ltype = lhs->getType(catalog, tableNameGetter);
		auto rtype = rhs->getType(catalog, tableNameGetter);
		if ((!ltype) || (!rtype)) {
			return nullptr;
		}
		
		if (ltype->promotableToInt() && rtype->promotableToInt()) {
			return ltype;
		}
		if (ltype->promotableToDouble() && rtype->promotableToDouble()) {
			return make_shared <MyDB_DoubleAttType>();
		}
		std::cout << "MinusOp wrong type" << std::endl;
		return nullptr;
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		vector<std::pair<string, string>> ret = lhs->getIdentifiers();
		auto r_identifiers = rhs->getIdentifiers();
		ret.insert(ret.end(), r_identifiers.begin(), r_identifiers.end());
		return ret;
	}

	~MinusOp () {}
};

class PlusOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	PlusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "+ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ltype = lhs->getType(catalog, tableNameGetter);
		auto rtype = rhs->getType(catalog, tableNameGetter);
//		if (lhs->getType(tableDef, allInfo) == "double" || rhs->getType(tableDef, allInfo) == "double")
//			return "double";
//		else
//			return "int";
//	}
		if ((!ltype) || (!rtype)) {
			return nullptr;
		}

		if (ltype->isBool() || rtype->isBool()) {
			std::cout << "PlusOp wrong type, both are Boolean type" << std::endl;
			return nullptr;
		}
		if (ltype->promotableToInt() && rtype->promotableToInt()) {
			return ltype;
		}
		if (ltype->promotableToDouble() && rtype->promotableToDouble()) {
			return make_shared <MyDB_DoubleAttType>();
		}
		if (ltype->promotableToDouble() || rtype->promotableToDouble()) {
			std::cout << "PlusOp wrong type, both are Double type" << std::endl;
			return nullptr;
		}
		return make_shared <MyDB_StringAttType>();
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		vector<std::pair<string, string>> ret = lhs->getIdentifiers();
		auto r_identifiers = rhs->getIdentifiers();
		ret.insert(ret.end(), r_identifiers.begin(), r_identifiers.end());
		return ret;
	}
	

	~PlusOp () {}
};

class TimesOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	TimesOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "* (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ltype = lhs->getType(catalog, tableNameGetter);
		auto rtype = rhs->getType(catalog, tableNameGetter);
		if ((!ltype) || (!rtype)) {
			return nullptr;
		}
		
		if (ltype->promotableToInt() && rtype->promotableToInt()) {
			return ltype;
		}
		if (ltype->promotableToDouble() && rtype->promotableToDouble()) {
			return make_shared <MyDB_DoubleAttType>();
		}
		std::cout << "TimesOp wrong type" << std::endl;
		return nullptr;
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		vector<std::pair<string, string>> ret = lhs->getIdentifiers();
		auto r_identifiers = rhs->getIdentifiers();
		ret.insert(ret.end(), r_identifiers.begin(), r_identifiers.end());
		return ret;
	}

	~TimesOp () {}
};

class DivideOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	DivideOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "/ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ltype = lhs->getType(catalog, tableNameGetter);
		auto rtype = rhs->getType(catalog, tableNameGetter);
		if ((!ltype) || (!rtype)) {
			return nullptr;
		}
		if (ltype->promotableToInt() && rtype->promotableToInt()) {
			return ltype;
		}
		if (ltype->promotableToDouble() && rtype->promotableToDouble()) {
			return make_shared <MyDB_DoubleAttType>();
		}
		std::cout << "DivideOP wrong type" << std::endl;
		return nullptr;
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		vector<std::pair<string, string>> ret = lhs->getIdentifiers();
		auto r_identifiers = rhs->getIdentifiers();
		ret.insert(ret.end(), r_identifiers.begin(), r_identifiers.end());
		return ret;
	}

	~DivideOp () {}
};

class GtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	GtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "> (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ltype = lhs->getType(catalog, tableNameGetter);
		auto rtype = rhs->getType(catalog, tableNameGetter);
		if ((!ltype) || (!rtype)) {
			return nullptr;
		}
		if (ltype->isBool() || rtype->isBool()) {
			std::cout << "GtOp wrong type, both are Boolean" << std::endl;
			return nullptr;
		}
		if (ltype->promotableToDouble() && rtype->promotableToDouble()) {
			return make_shared <MyDB_BoolAttType>();
		}
		if (ltype->promotableToDouble() || rtype->promotableToDouble()) {
			std::cout << "GtOp wrong type, at least one is numeric or string" << std::endl;
			return nullptr;
		}
		return make_shared <MyDB_BoolAttType>();
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		vector<std::pair<string, string>> ret = lhs->getIdentifiers();
		auto r_identifiers = rhs->getIdentifiers();
		ret.insert(ret.end(), r_identifiers.begin(), r_identifiers.end());
		return ret;
	}


	~GtOp () {}
};

class LtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	LtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "< (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ltype = lhs->getType(catalog, tableNameGetter);
		auto rtype = rhs->getType(catalog, tableNameGetter);
		if ((!ltype) || (!rtype)) {
			return nullptr;
		}
		if (ltype->isBool() || rtype->isBool()) {
			std::cout << "LtOp wrong type, one of it is boolean" << std::endl;
			return nullptr;
		}
		if (ltype->promotableToDouble() && rtype->promotableToDouble()) {
			return make_shared <MyDB_BoolAttType>();
		}
		if (ltype->promotableToDouble() || rtype->promotableToDouble()) {
			std::cout << "LtOp wrong type, at least one is numeric or string" << std::endl;
			return nullptr;
		}
		return make_shared <MyDB_BoolAttType>();
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		vector<std::pair<string, string>> ret = lhs->getIdentifiers();
		auto r_identifiers = rhs->getIdentifiers();
		ret.insert(ret.end(), r_identifiers.begin(), r_identifiers.end());
		return ret;
	}


	~LtOp () {}
};

class NeqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	NeqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "!= (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ltype = lhs->getType(catalog, tableNameGetter);
		auto rtype = rhs->getType(catalog, tableNameGetter);
		if ((!ltype) || (!rtype)) {
			return nullptr;
		}
		if (ltype->isBool() || rtype->isBool()) {
			std::cout << "NeqOp wrong type, at least one is boolean" << std::endl;
			return nullptr;
		}
		if (ltype->promotableToDouble() && rtype->promotableToDouble()) {
			return make_shared <MyDB_BoolAttType>();
		}
		if (ltype->promotableToDouble() || rtype->promotableToDouble()) {
			std::cout << "NeqOp wrong type, at least one is numeric or string" << std::endl;
			return nullptr;
		}
		return make_shared <MyDB_BoolAttType>();
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		vector<std::pair<string, string>> ret = lhs->getIdentifiers();
		auto r_identifiers = rhs->getIdentifiers();
		ret.insert(ret.end(), r_identifiers.begin(), r_identifiers.end());
		return ret;
	}

	~NeqOp () {}
};

class OrOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	OrOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "|| (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ltype = lhs->getType(catalog, tableNameGetter);
		auto rtype = rhs->getType(catalog, tableNameGetter);
		if ((!ltype) || (!rtype)) {
			return nullptr;
		}
		if (ltype->isBool() && rtype->isBool()) {
			return ltype;
		}
		std::cout << "OrOp wrong type" << std::endl;
		return nullptr;
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		vector<std::pair<string, string>> ret = lhs->getIdentifiers();
		auto r_identifiers = rhs->getIdentifiers();
		ret.insert(ret.end(), r_identifiers.begin(), r_identifiers.end());
		return ret;
	}

	~OrOp () {}
};

class EqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	EqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "== (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ltype = lhs->getType(catalog, tableNameGetter);
		auto rtype = rhs->getType(catalog, tableNameGetter);
		if ((!ltype) || (!rtype)) {
			return nullptr;
		}
		if (ltype->isBool() || rtype->isBool()) {
			std::cout << "EqOp wrong type, at least one is boolean" << std::endl;
			return nullptr;
		}
		if (ltype->promotableToDouble() && rtype->promotableToDouble()) {
			return make_shared <MyDB_BoolAttType>();
		}
		if (ltype->promotableToDouble() || rtype->promotableToDouble()) {
			std::cout << "EqOp wrong type, at least one is numeric or string" << std::endl;
			return nullptr;
		}
		return make_shared <MyDB_BoolAttType>();
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		vector<std::pair<string, string>> ret = lhs->getIdentifiers();
		auto r_identifiers = rhs->getIdentifiers();
		ret.insert(ret.end(), r_identifiers.begin(), r_identifiers.end());
		return ret;
	}	
	
	std::pair<ExprTreePtr, ExprTreePtr> getChildren() {
		return std::make_pair(lhs, rhs);
	}

	~EqOp () {}
};

class NotOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	NotOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "!(" + child->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ctype = child->getType(catalog, tableNameGetter);
		if (!ctype) {
			return nullptr;
		}
		if (ctype->isBool()) {
			return ctype;
		}
		std::cout << "NotOp wrong type" << std::endl;
		return nullptr;
	}	
	
	vector<std::pair<string, string>> getIdentifiers() override {
		return child->getIdentifiers();
	}

	~NotOp () {}
};

class SumOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	SumOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "sum(" + child->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ctype = child->getType(catalog, tableNameGetter);
		if (!ctype) {
			return nullptr;
		}
		if (ctype->promotableToDouble()) {
			return ctype;
		}
		std::cout << "SumOp wrong type" << std::endl;
		return nullptr;
	}	
	
	vector<std::pair<string, string>> getIdentifiers() override {
		return child->getIdentifiers();
	}

	~SumOp () {}
};

class AvgOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	AvgOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "avg(" + child->toString () + ")";
	}	
	
	MyDB_AttTypePtr getType(MyDB_CatalogPtr catalog, vector <pair <string, string>>& tableNameGetter) override {
		auto ctype = child->getType(catalog, tableNameGetter);
		if (!ctype) {
			return nullptr;
		}
		if (ctype->promotableToDouble()) {
			return std::make_shared<MyDB_DoubleAttType>();
		}
		std::cout << "AvgOp wrong type" << std::endl;
		return nullptr;
	}
	
	vector<std::pair<string, string>> getIdentifiers() override {
		return child->getIdentifiers();
	}

	~AvgOp () {}
};

#endif
