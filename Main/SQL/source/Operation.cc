#include "operation.h"

operation::operation(SQLStatement *query, MyDB_BufferManagerPtr buffer,
                     map<string, MyDB_TableReaderWriterPtr> tables, MyDB_CatalogPtr catalog)
{
    rem = 0;
    this->query = query->getSFWQuery();
    this->buffer = buffer;
    this->tables = tables;
    this->schemaOut = make_shared<MyDB_Schema> ();
    this->schemaSp = make_shared<MyDB_Schema> ();
    this->cata = catalog;
    int count = 0;
    for (auto s : this->query.valuesToSelect) {

        schemaOut->appendAtt(s->getAttSchema(to_string(count)));
        projection.push_back(s->toString());
        schemaSp->appendAtt(s->getAttSchema(to_string(count)));

        if (s->getAttSchema(to_string(count)).first.substr(0,3) == "sum") {
            aggsToCompute.push_back(make_pair(MyDB_AggType::sumA, s->toString().substr(3)));
        } else if (s->getAttSchema(to_string(count)).first.substr(0,3) == "avg") {
            aggsToCompute.push_back(make_pair(MyDB_AggType::avgA, s->toString().substr(3)));
        } else {
            groupings.push_back(s->toString());
            rem = count;
        }

        count++;

    }
    if (aggsToCompute.empty()) {
        this->isAgg = false;
    } else {
        this->isAgg = true;
        if (!groupings.empty()) {
            this->sp = true;
            vector<pair<string, MyDB_AttTypePtr>> sch(schemaOut->getAtts());
            auto& atts = schemaOut->getAtts();
            atts.erase(atts.begin()+rem);
            atts.emplace(atts.begin(), sch[rem]);
        }

    }
}

MyDB_TableReaderWriterPtr operation::copyyyy(MyDB_TableReaderWriterPtr input, string alias, string name) {
    MyDB_TablePtr temp = make_shared<MyDB_Table>();
    temp->fromCatalog(name, cata);
    vector<pair<string, MyDB_AttTypePtr>> sch(input->getTable()->getSchema()->getAtts());
    auto& tempSch = temp->getSchema()->getAtts();
    tempSch.clear();
    for (auto s : sch) {
        tempSch.emplace_back(make_pair(alias + "_" + s.first, s.second));
    }
    cout << temp->getTupleCount() << endl;
    return make_shared<MyDB_TableReaderWriter>(temp, buffer);
}

void operation::run() {
    vector<pair<string, string> >& tablesToProcess = query.tablesToProcess;
    map<string, MyDB_TableReaderWriterPtr> inputTemp;
    MyDB_TableReaderWriterPtr input;
    MyDB_TableReaderWriterPtr finalInput;
    MyDB_TablePtr outputTable = make_shared<MyDB_Table>("output", "output.bin", schemaOut);
    MyDB_TableReaderWriterPtr output = make_shared<MyDB_TableReaderWriter>(outputTable, buffer);
    if (tablesToProcess.size() != 1) {
        // TODO: join and result in finalInput.
        cout << "Join and process" << endl;
    } else {

        input = tables[tablesToProcess[0].first];
        finalInput = copyyyy(input, tablesToProcess[0].second, tablesToProcess[0].first);
    }

    string predicates = "";
    int i = 0;
    for (auto p : query.allDisjunctions) {
        if (i == 0) {
            predicates += p->toString();
        } else {
            predicates = "&& (" + predicates + "," + p->toString() + ")";
        }
        ++i;
    }

    if (isAgg) {
        Aggregate op(finalInput, output, aggsToCompute, groupings, predicates);
        op.run();
    } else {
        RegularSelection op(finalInput, output, predicates, projection);
        op.run();
    }

    //Output
    if (!sp) {
        int count = 0;
        MyDB_RecordPtr recOut = output->getEmptyRecord();
        MyDB_RecordIteratorAltPtr it = output->getIteratorAlt();

        printf("----------------Results Below---------------\n");
        while (it->advance()) {
            it->getCurrent(recOut);
            ++count;
            if (count <= 30) {
                cout << recOut << endl;
            }
        }
        printf("Count Result: %d records.\n", count);
    } else {
        int count = 0;
        MyDB_TablePtr outputTableSp = make_shared<MyDB_Table>("outputSp", "outputSp.bin", schemaSp);
        MyDB_TableReaderWriterPtr outputSp = make_shared<MyDB_TableReaderWriter>(outputTableSp, buffer);
        vector<string> projectionSp;

        for (auto m : schemaSp->getAtts()) {
            projectionSp.push_back("["+m.first+"]");
        }

        RegularSelection opSp(output, outputSp, "== (string[T], string[T])", projectionSp);
        opSp.run();
        count = 0;
        MyDB_RecordPtr recOutt = outputSp->getEmptyRecord();
        MyDB_RecordIteratorAltPtr itt = outputSp->getIteratorAlt();

        printf("----------------Results Below---------------\n");
        while (itt->advance()) {
            itt->getCurrent(recOutt);
            ++count;
            if (count <= 30) {
                cout << recOutt << endl;
            }
        }
        printf("Count Result: %d records.\n", count);
    }
}