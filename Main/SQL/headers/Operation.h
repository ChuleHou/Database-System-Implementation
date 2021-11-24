#ifndef A7_OPERATION_H
#define A7_OPERATION_H

#endif //A7_OPERATION_H
#include "ParserTypes.h"
#include "MyDB_TableReaderWriter.h"
#include "Aggregate.h"
#include "RegularSelection.h"
#include "ScanJoin.h"

class operation
{
private:
    /* data */
    SFWQuery query;
    map<string, MyDB_TableReaderWriterPtr> tables;
    MyDB_BufferManagerPtr buffer;
    vector<string> groupings;
    vector<pair<MyDB_AggType, string>> aggsToCompute;
    vector<string> projection;
    MyDB_SchemaPtr schemaOut;
    MyDB_SchemaPtr schemaSp;
    MyDB_CatalogPtr cata;
    bool isAgg;
    bool sp;
    int rem;

public:
    operation(SQLStatement *query, MyDB_BufferManagerPtr buffer,
              map<string, MyDB_TableReaderWriterPtr> tables, MyDB_CatalogPtr catalog);
    void run();
    MyDB_TableReaderWriterPtr copyyyy(MyDB_TableReaderWriterPtr input, string alias, string name);
};