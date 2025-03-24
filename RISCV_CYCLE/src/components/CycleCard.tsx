
import React from "react";
import StageItem from "./StageItem";
import { Card, CardHeader, CardTitle, CardContent } from "@/components/ui/card";

interface Log {
  stage: string;
  message: string;
}

interface Register {
  reg: string;
  value: string;
}

interface Memory {
  address: string;
  value: string;
}

interface CycleCardProps {
  cycle: number;
  logs: Log[];
  registers?: Register[];
  memory?: Memory[];
}

const CycleCard: React.FC<CycleCardProps> = ({ cycle, logs, registers, memory }) => {
  // Filter for the five main stages (or use default message if not found)
  const getStageMessage = (stageName: string) => {
    const log = logs.find(log => log.stage.toLowerCase() === stageName.toLowerCase());
    return log ? log.message : "No data available";
  };

  return (
    <Card className="animate-scale-in transition-shadow duration-300 hover:shadow-md mb-6">
      <CardHeader className="pb-2">
        <CardTitle className="text-xl font-semibold">Cycle {cycle}</CardTitle>
      </CardHeader>
      <CardContent className="pt-0">
        <div className="grid grid-cols-1 md:grid-cols-12 gap-4">
          {/* Pipeline Stages - Left Column (4 cols on md+) */}
          <div className="md:col-span-4 space-y-2">
            <StageItem stage="Fetch" message={getStageMessage("fetch")} />
            <StageItem stage="Decode" message={getStageMessage("decode")} />
            <StageItem stage="Execute" message={getStageMessage("execute")} />
            <StageItem stage="Memory" message={getStageMessage("memory")} />
            <StageItem stage="WriteBack" message={getStageMessage("writeback")} />
          </div>
          
          {/* Registers - Middle Column (4 cols on md+) */}
          <div className="md:col-span-4">
            {registers && registers.length > 0 ? (
              <div className="border border-border/30 rounded-md p-2 h-full">
                <h3 className="text-sm font-medium mb-2 pb-1 border-b">Registers</h3>
                <div className="grid grid-cols-1 gap-1.5 text-xs overflow-y-auto max-h-[300px]">
                  {registers.map((reg, index) => (
                    <div key={index} className="flex justify-between bg-secondary/20 p-1.5 rounded">
                      <span className="font-mono mr-1">{reg.reg}:</span>
                      <span className="font-mono truncate">{reg.value}</span>
                    </div>
                  ))}
                </div>
              </div>
            ) : (
              <div className="border border-border/30 rounded-md p-2 h-full flex items-center justify-center">
                <p className="text-sm text-muted-foreground">No register data available</p>
              </div>
            )}
          </div>
          
          {/* Memory - Right Column (4 cols on md+) */}
          <div className="md:col-span-4">
            {memory && memory.length > 0 ? (
              <div className="border border-border/30 rounded-md p-2 h-full">
                <h3 className="text-sm font-medium mb-2 pb-1 border-b">Memory</h3>
                <div className="grid grid-cols-1 gap-1.5 text-xs overflow-y-auto max-h-[300px]">
                  {memory.map((mem, index) => (
                    <div key={index} className="flex justify-between bg-secondary/20 p-1.5 rounded">
                      <span className="font-mono mr-1">{mem.address}:</span>
                      <span className="font-mono truncate">{mem.value}</span>
                    </div>
                  ))}
                </div>
              </div>
            ) : (
              <div className="border border-border/30 rounded-md p-2 h-full flex items-center justify-center">
                <p className="text-sm text-muted-foreground">No memory data available</p>
              </div>
            )}
          </div>
        </div>
      </CardContent>
    </Card>
  );
};

export default CycleCard;
