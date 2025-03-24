
import React from "react";
import { cn } from "@/lib/utils";

interface StageItemProps {
  stage: string;
  message: string;
}

const StageItem: React.FC<StageItemProps> = ({ stage, message }) => {
  // Normalize stage name for CSS class (lowercase, remove spaces)
  const normalizedStage = stage.toLowerCase().replace(/\s+/g, "");
  
  return (
    <div className={cn(
      "rounded-md p-3 transition-colors duration-300 border",
      {
        "bg-fetch-light dark:bg-fetch-dark border-fetch-text-light/30 dark:border-fetch-text-dark/30": normalizedStage === "fetch",
        "bg-decode-light dark:bg-decode-dark border-decode-text-light/30 dark:border-decode-text-dark/30": normalizedStage === "decode",
        "bg-execute-light dark:bg-execute-dark border-execute-text-light/30 dark:border-execute-text-dark/30": normalizedStage === "execute",
        "bg-memory-light dark:bg-memory-dark border-memory-text-light/30 dark:border-memory-text-dark/30": normalizedStage === "memory",
        "bg-writeback-light dark:bg-writeback-dark border-writeback-text-light/30 dark:border-writeback-text-dark/30": normalizedStage === "writeback",
        "bg-secondary/30 dark:bg-secondary/40 border-border/30": !["fetch", "decode", "execute", "memory", "writeback"].includes(normalizedStage)
      }
    )}>
      <div className={cn(
        "text-sm font-semibold mb-1",
        {
          "text-fetch-text-light dark:text-fetch-text-dark": normalizedStage === "fetch",
          "text-decode-text-light dark:text-decode-text-dark": normalizedStage === "decode",
          "text-execute-text-light dark:text-execute-text-dark": normalizedStage === "execute",
          "text-memory-text-light dark:text-memory-text-dark": normalizedStage === "memory",
          "text-writeback-text-light dark:text-writeback-text-dark": normalizedStage === "writeback"
        }
      )}>
        {stage}
      </div>
      <div className="text-sm font-mono text-foreground/90 break-words">
        {message}
      </div>
    </div>
  );
};

export default StageItem;
