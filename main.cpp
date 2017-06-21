#include <QCoreApplication>
#include <QFile>
//#include <QDataStream>
#include <QTextStream>
#include <QTimer>
//#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
//#include <QSettings>
#include <QVector>
#include <QProcess>
#include <QProcessEnvironment>
#include <QCommandLineParser>
#include <QSet>

namespace
{
struct configFile_s
{
    QString cygcheckPath_pub;
    QString cygcheckDependencyNotFoundError_pub = "cygcheck: track_down: could not find ";
    QString windeployqtPath_pub;
    QJsonArray includeDllPaths_pub;
    QJsonArray excludeDllPaths_pub;
    void read(const QJsonObject &json)
    {
        cygcheckPath_pub = json["cygcheckPath"].toString();
        windeployqtPath_pub = json["windeployqtPath"].toString();
        if (not json["cygcheckDependencyNotFoundError"].isUndefined())
        {
            cygcheckDependencyNotFoundError_pub = json["cygcheckDependencyNotFoundError"].toString();
        }
        includeDllPaths_pub = json["includeDllPaths"].toArray();
        excludeDllPaths_pub = json["excludeDllPaths"].toArray();
    }
};

}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QString errorStr;
    QCoreApplication::setApplicationName("DllGatherer");
    QCoreApplication::setApplicationVersion("1.0");
    //Connect the stdout to my qout textstream
    QTextStream qout(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription("DllGatherer description");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("target", "Executable target to gather the dlls");

    // Process the actual command line arguments given by the user
    parser.process(app);

    while (errorStr.isEmpty())
    {
        const QStringList parsedPositionalArgs(parser.positionalArguments());
        if (parsedPositionalArgs.size() < 1)
        {
            errorStr.append("No executable target to gather dlls for");
            break;
        }

        QString fileToGatherDllsStr(parsedPositionalArgs.at(0));
        QFileInfo fileToGatherDllsFileInfo(QDir::fromNativeSeparators(fileToGatherDllsStr));
        if (not fileToGatherDllsFileInfo.exists())
        {
            errorStr.append("Target to gather dlls for doesn't exist");
            break;
        }

        auto sysEnviroment(QProcessEnvironment::systemEnvironment());
        QString PATHStr(sysEnviroment.value("PATH"));
    //    qout << R"(sysEnviroment.value("PATH") )" << PATHStr << endl;
    #ifdef __WIN32__
        auto splittedPATH(PATHStr.split(";", QString::SkipEmptyParts));
    #else
        auto splittedPATH(PATHStr.split(":", QString::SkipEmptyParts));
    #endif

       QSet<QString> PATHItemsSet;
       for (const auto& pathItem_ite_con : splittedPATH)
       {
           //qout << "QDir::fromNativeSeparators(pathItem_ite_con) " << QDir::fromNativeSeparators(pathItem_ite_con) << endl;
           PATHItemsSet.insert(QDir::fromNativeSeparators(pathItem_ite_con));
       }

        QFile configFile("config.json");
        if (not configFile.exists())
        {
            errorStr.append("Config file, config.json, doesn't exist");
            break;
        }
        QByteArray jsonByteArray;
        if (configFile.open(QIODevice::ReadOnly))
        {
            jsonByteArray = configFile.readAll();
        }
        else
        {
            errorStr.append("Could not open config file config.json");
            break;
        }
        auto jsonDocObj(QJsonDocument::fromJson(jsonByteArray));
        configFile_s config;
        if (jsonDocObj.isNull())
        {
            errorStr.append("Could not parse json from the config file, config.json");
            //qout << "jsonByteArray " << jsonByteArray << endl;
            //qout << "isNull jsonDocObj" << endl;
            break;
        }
        else
        {
            config.read(jsonDocObj.object());
//            qout << "configFile.cygcheckLocation_pub " << config.cygcheckPath_pub << endl;
//            for (const auto& dllFolder_ite_con : config.dllPaths_pub)
//            {
//                qout << "dllFolder_ite_con " << dllFolder_ite_con.toString() << endl;
//            }
        }

        if (config.cygcheckPath_pub.isEmpty())
        {
            errorStr.append("cycgcheckPath string is not set or empty in the config.json file");
            break;
        }

        for (const auto& ddlPath_ite_con : config.includeDllPaths_pub.toVariantList())
        {
            QDir qdirTmp(QDir::fromNativeSeparators(ddlPath_ite_con.toString()));
            if (qdirTmp.exists())
            {
                //qout << "qdirTmp.canonicalPath() " << qdirTmp.canonicalPath() << endl;
                PATHItemsSet.insert(qdirTmp.canonicalPath());
            }
        }
        QSet<QString> excludeDllPathsSet;
        for (const auto& ddlPath_ite_con : config.excludeDllPaths_pub.toVariantList())
        {
            QDir qdirTmp(QDir::fromNativeSeparators(ddlPath_ite_con.toString()));
            excludeDllPathsSet.insert(qdirTmp.canonicalPath());
        }
        for (const auto& excludeDllPath_ite_con : excludeDllPathsSet)
        {
            PATHItemsSet.remove(excludeDllPath_ite_con);
        }

//        for (const auto& pathItem_ite_con : PATHItemsSet)
//        {
//            qout << "pathItem_ite_con " << pathItem_ite_con << endl;
//        }
        QStringList oneArgument({R"(.\)" + fileToGatherDllsFileInfo.fileName()});
        if (not config.windeployqtPath_pub.isEmpty())
        {
            QProcess windeployqtProcess;
            windeployqtProcess.setWorkingDirectory(fileToGatherDllsFileInfo.canonicalPath());
            windeployqtProcess.start(config.windeployqtPath_pub, oneArgument);
            if (not windeployqtProcess.waitForFinished())
            {
                errorStr.append(windeployqtProcess.errorString());
            }
            if (windeployqtProcess.exitCode() != EXIT_SUCCESS)
            {
                errorStr.append(config.windeployqtPath_pub + " failed, return code: " + QString::number(windeployqtProcess.exitCode()));
            }
            qout << windeployqtProcess.readAll() << endl;
            if (not errorStr.isEmpty())
            {
                break;
            }
        }

        bool copiesWereMade(true);
        while (copiesWereMade and errorStr.isEmpty())
        {
            copiesWereMade = false;
            QProcess cygcheckProcess;
            cygcheckProcess.setWorkingDirectory(fileToGatherDllsFileInfo.canonicalPath());
            //cygcheckProcess.setProcessChannelMode(QProcess::MergedChannels);
            cygcheckProcess.start(config.cygcheckPath_pub, oneArgument);
            if (not cygcheckProcess.waitForFinished())
            {
                errorStr.append(cygcheckProcess.errorString());
            }

            QSet<QString> checkResultFileNames;
            QString processStderrOutput(cygcheckProcess.readAllStandardError());
            if (not processStderrOutput.isEmpty())
            {
                if (processStderrOutput.contains(config.cygcheckDependencyNotFoundError_pub))
                {
    //                qout << "processStderrOutput.contains(errorFindingDependencyFilename) " << endl;
                    QStringList checkResultFilenames(processStderrOutput.split("\n", QString::SkipEmptyParts));
                    for (const auto& checkResultFilename_ite_con : checkResultFilenames)
                    {
                        if (checkResultFilename_ite_con.startsWith(config.cygcheckDependencyNotFoundError_pub))
                        {
                            //qout << "checkResultFilename_ite_con.mid(errorFindingDependencyFilename.size()) " << checkResultFilename_ite_con.mid(errorFindingDependencyFilename.size()) << endl;
                            QString substrTmp(checkResultFilename_ite_con.mid(config.cygcheckDependencyNotFoundError_pub.size()));
                            checkResultFileNames.insert(substrTmp.trimmed());
                        }
                    }
                }
                else
                {
                    errorStr.append(processStderrOutput);
                }
            }
            if (cygcheckProcess.exitCode() != EXIT_SUCCESS)
            {
                errorStr.append(config.cygcheckPath_pub + " failed, return code: " + QString::number(cygcheckProcess.exitCode()));
            }
            if (not errorStr.isEmpty())
            {
                break;
            }

            QString result(cygcheckProcess.readAll());
            //qout << "process result " << result << endl;

            QStringList checkResultFullPaths(result.split("\n", QString::SkipEmptyParts));
            //the first line is the actual path to the executable, the output is like a tree
            //and the root, the first line, is the executable
            checkResultFullPaths.removeFirst();

            QSet<QString> filesToCopy;
            //qout << "check for fullPaths in PATHs " << endl;
            for (auto i = 0, l = checkResultFullPaths.size(); i < l; ++i)
            {
                //qout << "dll dependency found on PATH " << resultLines[i] << "\n";
                checkResultFullPaths[i] = QDir::fromNativeSeparators(checkResultFullPaths.at(i).trimmed());
                if (checkResultFullPaths[i].isEmpty())
                {
                    continue;
                }
                QFileInfo resultFileInfoTmp(checkResultFullPaths[i]);
                //if the dll is on the same path ignore it
                if (fileToGatherDllsFileInfo.canonicalPath() == resultFileInfoTmp.canonicalPath())
                {
                    continue;
                }
                if (not resultFileInfoTmp.exists())
                {
                    errorStr.append("Dependency file " + checkResultFullPaths[i] + " doesn't exist\n");
                }
                else
                {
                    QDir resultPathTmp(resultFileInfoTmp.canonicalPath());
                    if (not resultPathTmp.exists())
                    {
                        errorStr.append("Dependency path " + resultPathTmp.canonicalPath() + " doesn't exist\n");
                    }
                    else
                    {
                        auto findResultTmp(PATHItemsSet.find(resultPathTmp.canonicalPath()));
                        //if it's on the path folders do nothing
                        if (findResultTmp != PATHItemsSet.constEnd())
                        {
                            //qout << "pathTmp.canonicalPath() " << pathTmp.canonicalPath() << "\n";
                            //qout << "*findResultTmp " << *findResultTmp << "\n";
                            //qout << "dll dependency found on PATH " << resultLines[i] << "\n";
                        }
                        else
                        {
                            //not found on the PATH it has to be copied, if the folder
                            //isn't on the excluded dll paths
                            auto findExcludedResultTmp(excludeDllPathsSet.find(resultFileInfoTmp.canonicalPath()));
                            if (findExcludedResultTmp == excludeDllPathsSet.constEnd())
                            {
                                filesToCopy.insert(checkResultFullPaths.at(i));
                            }
                            else
                            {
                                checkResultFileNames.insert(resultFileInfoTmp.fileName());
                            }
                        }
                    }
                }
            }

            //qout << "check for filenames in PATHs " << endl;
            for (const auto& checkResultFilename_ite_con : checkResultFileNames)
            {
                //qout << "checkResultFilename_ite_con " << checkResultFilename_ite_con << "\n";
                //although i don't think it would complain about not finding on the same executable path
                //check if it's already there
                QFileInfo fileInfoTmp(fileToGatherDllsFileInfo.canonicalPath() + R"(/)" + checkResultFilename_ite_con);
                //if it's there no need to copy it again
                if (fileInfoTmp.exists())
                {
                    //qout << "checkResultFilename_ite_con exists" << endl;
                    continue;
                }

                for (const auto& pathItem_ite_con : PATHItemsSet)
                {
                    QFileInfo fileInfoPathTmp(pathItem_ite_con + R"(/)" + checkResultFilename_ite_con);
                    //qout << "fileInfoPathTmp " << pathItem_ite_con + R"(/)" + checkResultFilename_ite_con << "\"" << endl;
                    if (fileInfoPathTmp.exists())
                    {
                        //qout << "fileInfoPathTmp.exists()" << endl;
                        filesToCopy.insert(fileInfoPathTmp.canonicalFilePath());
                        break;
                    }
                }
            }

            //qout << "copy files " << endl;
            for (const auto& fileToCopy_ite_con : filesToCopy)
            {
                copiesWereMade = true;
                //qout << "copying " << fileToCopy_ite_con << "\n";
                QFileInfo sourceFileInfoTmp(fileToCopy_ite_con);
                if (QFile::copy(fileToCopy_ite_con, fileToGatherDllsFileInfo.canonicalPath() + R"(/)" + sourceFileInfoTmp.fileName()))
                {
                    qout << "Copied " << fileToCopy_ite_con << " to " << fileToGatherDllsFileInfo.canonicalPath() << R"(/)" << sourceFileInfoTmp.fileName() << "\n";
                }
            }
    //        for (const auto& lineItem_ite_con : resultLines)
    //        {
    //            qout << "lineItem_ite_con " << lineItem_ite_con << endl;
    //        }

        }
        break;
    }

    QTimer::singleShot(0, &app, SLOT(quit()));

    app.exec(); //run the application

    if (not errorStr.isEmpty())
    {
        qout << "Errors:\n" << errorStr << endl;
        return EXIT_FAILURE;
    }
}
